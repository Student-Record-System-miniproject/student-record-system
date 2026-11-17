#include "srm/grade_service.h"
#include "srm/report_export.h"
#include "srm/rbac.h"
#include "srm/storage_utils.h"
#include "srm/input_validator.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>
#include <iomanip>
#include <map>
#include <unordered_map>

namespace srm {
namespace {

OperationResult failure(ErrorCode error, std::string message) {
    return {false, error, std::move(message)};
}

struct GradeInfo {
    std::string letterGrade;
    double gradePoint = 0.0;
};

GradeInfo calculateGrade(double marks) {
    if (marks >= 90) return {"S", 10.0};
    if (marks >= 80) return {"A", 9.0};
    if (marks >= 70) return {"B", 8.0};
    if (marks >= 60) return {"C", 7.0};
    if (marks >= 50) return {"D", 6.0};
    if (marks >= 40) return {"E", 5.0};
    return {"F", 0.0};
}

using SemesterRecords =
    std::map<int, std::vector<const MarkRecord*>>;

struct OverallGradeTotals {
    int totalMarks = 0;
    int subjectCount = 0;
    double totalGradePoints = 0.0;
};

SemesterRecords collectSemesterRecords(
    const std::vector<MarkRecord>& records,
    const std::string& rollNumber) {

    SemesterRecords semesterRecords;

    for (const MarkRecord& record : records) {
        if (record.rollNumber == rollNumber) {
            semesterRecords[record.semester].push_back(&record);
        }
    }

    return semesterRecords;
}

void appendSemesterReport(
    std::ostringstream& report,
    int semesterNumber,
    const std::vector<const MarkRecord*>& records,
    OverallGradeTotals& overall) {

    int semesterTotal = 0;
    double semesterGradePoints = 0.0;

    report << "\nSemester " << semesterNumber << '\n';

    for (const MarkRecord* record : records) {
        const GradeInfo grade = calculateGrade(record->marks);

        report << record->subject
               << " | Marks " << record->marks
               << " | Grade " << grade.letterGrade
               << " | Grade Points " << grade.gradePoint << '\n';

        semesterTotal += record->marks;
        semesterGradePoints += grade.gradePoint;
    }

    const int subjectCount = static_cast<int>(records.size());

    const double percentage =
        static_cast<double>(semesterTotal) / subjectCount;

    const double gpa = semesterGradePoints / subjectCount;

    const GradeInfo semesterGrade = calculateGrade(percentage);

    report << "Semester Percentage: " << percentage << "%\n";
    report << "Semester Grade: " << semesterGrade.letterGrade << '\n';
    report << "Semester GPA: " << gpa << '\n';

    overall.totalMarks += semesterTotal;
    overall.subjectCount += subjectCount;
    overall.totalGradePoints += semesterGradePoints;
}

void appendOverallResult(
    std::ostringstream& report,
    const OverallGradeTotals& overall) {

    const double percentage =
        static_cast<double>(overall.totalMarks) / overall.subjectCount;

    const double gpa =
        overall.totalGradePoints / overall.subjectCount;

    const GradeInfo grade = calculateGrade(percentage);

    report << "\nOverall Result\n";
    report << "Overall Percentage: " << percentage << "%\n";
    report << "Overall Grade: " << grade.letterGrade << '\n';
    report << "Overall GPA: " << gpa << '\n';
}

}  // namespace

GradeService::GradeService(std::filesystem::path gradeFile,
                           ExistsCheck studentExists,
                           ExistsCheck subjectExists)
    : repository_(std::move(gradeFile)),
      studentExists_(std::move(studentExists)),
      subjectExists_(std::move(subjectExists)),
      storageReady_(repository_.load()) {}

OperationResult GradeService::enterOrUpdateMarks(
    const Session& session,
    const std::string& rollNumber,
    const std::string& subject,
    int semester,
    const std::string& marksInput) {
    const auto auth = AccessController::authorize(session, Operation::EnterMarks);
    if (!auth.success) {
        return auth;
    }
    if (!storageReady_) {
        return failure(ErrorCode::StorageFailure, "Grade data could not be loaded.");
    }
    if (!InputValidator::validateText(rollNumber) || !InputValidator::validateText(subject) || semester <= 0) {
        return failure(ErrorCode::InvalidInput,
                       "Student, subject, and semester must be valid.");
    }
    if (!studentExists_ || !studentExists_(rollNumber)) {
        return failure(ErrorCode::StudentNotFound, "Student was not found.");
    }
    if (!subjectExists_ || !subjectExists_(subject)) {
        return failure(ErrorCode::SubjectNotFound, "Subject was not found.");
    }

    int marks = 0;
    if (!InputValidator::parseInteger(marksInput, marks) || !InputValidator::validateRange(marks, 0, 100)) {
        return failure(ErrorCode::InvalidInput,
                       "Marks must be an integer from 0 through 100.");
    }

    const auto previousRecords = repository_.snapshot();
    repository_.upsert({rollNumber, subject, semester, marks});
    if (!repository_.save()) {
        repository_.restore(previousRecords);
        storageReady_ = false;
        return failure(ErrorCode::StorageFailure, "Grade data could not be saved.");
    }
    return {true, ErrorCode::None, "Grade saved."};
}

ReportResult GradeService::generateReportCard(
    const Session& session,
    const std::string& rollNumber) const {

    const auto op = (session.role == Role::Student)
                        ? Operation::ViewOwnReportCard
                        : Operation::ViewAnyStudentReportCard;

    const auto auth = AccessController::authorize(session, op, rollNumber);

    if (!auth.success) {
        return {false, auth.error, auth.message, {}};
    }

    if (!InputValidator::validateText(rollNumber)) {
        return {false, ErrorCode::InvalidInput,
                "Invalid student roll number.", {}};
    }

    if (!studentExists_ || !studentExists_(rollNumber)) {
        return {false, ErrorCode::StudentNotFound,
                "Student was not found.", {}};
    }

    std::ostringstream report;
    report << std::fixed << std::setprecision(2);
    report << "Report card for " << rollNumber << '\n';

    const auto semesters =
        collectSemesterRecords(repository_.records(), rollNumber);

    if (semesters.empty()) {
        report << "No marks available.\n";
        return {true, ErrorCode::None, "Report generated.", report.str()};
    }

    OverallGradeTotals overall;

    for (const auto& [semesterNumber, records] : semesters) {
        appendSemesterReport(report, semesterNumber, records, overall);
    }

    appendOverallResult(report, overall);

    return {true, ErrorCode::None, "Report generated.", report.str()};
}

OperationResult GradeService::exportReportCard(
    const Session& session,
    const std::string& rollNumber,
    const std::filesystem::path& outputFile) const {
    return exportTextReport(generateReportCard(session, rollNumber), outputFile);
}

bool GradeService::deleteStudentGrades(const std::string& rollNumber) {
    if (!InputValidator::validateText(rollNumber)) {
        return false;
    }
    return repository_.removeByRollNumber(rollNumber);
}

}  // namespace srm

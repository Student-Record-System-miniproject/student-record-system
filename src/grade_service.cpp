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

namespace srm {
namespace {

OperationResult failure(ErrorCode error, std::string message) {
    return {false, error, std::move(message)};
}

struct GradeInfo {
    std::string letterGrade;
    double gradePoint;
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

}  // namespace

GradeRepository::GradeRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool GradeRepository::load() {
    recoverInterruptedSave(filePath_);
    records_.clear();
    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        MarkRecord record;
        std::string semester;
        std::string marks;
        if (!std::getline(row, record.rollNumber, '\t') ||
            !std::getline(row, record.subject, '\t') ||
            !std::getline(row, semester, '\t') ||
            !std::getline(row, marks) ||
            !InputValidator::validateText(record.rollNumber) ||
            !InputValidator::validateText(record.subject) ||
            !InputValidator::parseInteger(semester, record.semester) ||
            !InputValidator::parseInteger(marks, record.marks) ||
            record.semester <= 0 ||
            !InputValidator::validateRange(record.marks, 0, 100)) {
            records_.clear();
            return false;
        }
        upsert(std::move(record));
    }
    return true;
}

bool GradeRepository::save() const {
    return atomicWriteFile(filePath_, [&](std::ostream& output) {
        for (const MarkRecord& record : records_) {
            output << record.rollNumber << '\t' << record.subject << '\t'
                   << record.semester << '\t' << record.marks << '\n';
        }
        return output.good();
    });
}

void GradeRepository::upsert(MarkRecord record) {
    for (MarkRecord& existing : records_) {
        if (existing.rollNumber == record.rollNumber &&
            existing.subject == record.subject &&
            existing.semester == record.semester) {
            existing.marks = record.marks;
            return;
        }
    }
    records_.push_back(std::move(record));
}

std::vector<MarkRecord> GradeRepository::snapshot() const {
    return records_;
}

void GradeRepository::restore(std::vector<MarkRecord> records) {
    records_ = std::move(records);
}

bool GradeRepository::removeByRollNumber(const std::string& rollNumber) {
    const auto prevRecords = records_;
    const auto initialSize = records_.size();
    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const MarkRecord& r) { return r.rollNumber == rollNumber; }),
        records_.end());
    if (records_.size() != initialSize) {
        if (!save()) {
            records_ = prevRecords;
            return false;
        }
    }
    return true;
}

const std::vector<MarkRecord>& GradeRepository::records() const {
    return records_;
}

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

    std::map<int, std::vector<const MarkRecord*>> semesterRecords;

    int overallTotal = 0;
    int overallSubjects = 0;
    double overallGradePoints = 0.0;

    for (const MarkRecord& record : repository_.records()) {
        if (record.rollNumber == rollNumber) {
            semesterRecords[record.semester].push_back(&record);
        }
    }

    if (semesterRecords.empty()) {
        report << "No marks available.\n";
        return {true, ErrorCode::None, "Report generated.", report.str()};
    }

    // Calculate results for each semester.
    for (const auto& semester : semesterRecords) {
        int semesterTotal = 0;
        double semesterGradePoints = 0.0;

        report << "\nSemester " << semester.first << '\n';

        for (const MarkRecord* record : semester.second) {
            const GradeInfo grade = calculateGrade(record->marks);

            report << record->subject
                   << " | Marks " << record->marks
                   << " | Grade " << grade.letterGrade
                   << " | Grade Points " << grade.gradePoint << '\n';

            semesterTotal += record->marks;
            semesterGradePoints += grade.gradePoint;
        }

        const int subjectCount =
            static_cast<int>(semester.second.size());

        const double percentage =
            static_cast<double>(semesterTotal) / subjectCount;

        const double gpa =
            semesterGradePoints / subjectCount;

        const GradeInfo overallSemesterGrade =
            calculateGrade(percentage);

        report << "Semester Percentage: " << percentage << "%\n";
        report << "Semester Grade: "
               << overallSemesterGrade.letterGrade << '\n';
        report << "Semester GPA: " << gpa << '\n';

        overallTotal += semesterTotal;
        overallSubjects += subjectCount;
        overallGradePoints += semesterGradePoints;
    }

    // Calculate the overall result across all semesters.
    const double overallPercentage =
        static_cast<double>(overallTotal) / overallSubjects;

    const double overallGPA =
        overallGradePoints / overallSubjects;

    const GradeInfo overallGrade =
        calculateGrade(overallPercentage);

    report << "\nOverall Result\n";
    report << "Overall Percentage: "
           << overallPercentage << "%\n";
    report << "Overall Grade: "
           << overallGrade.letterGrade << '\n';
    report << "Overall GPA: "
           << overallGPA << '\n';

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

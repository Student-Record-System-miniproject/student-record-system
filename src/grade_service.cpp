#include "srm/grade_service.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {
namespace {

OperationResult failure(ErrorCode error, std::string message) {
    return {false, error, std::move(message)};
}

bool parseInteger(const std::string& input, int& value) {
    if (input.empty()) {
        return false;
    }
    const char* first = input.data();
    const char* last = first + input.size();
    const auto parsed = std::from_chars(first, last, value);
    return parsed.ec == std::errc{} && parsed.ptr == last;
}

bool validField(const std::string& value) {
    return !value.empty() && value.find('\t') == std::string::npos &&
           value.find('\n') == std::string::npos;
}

}  // namespace

GradeRepository::GradeRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool GradeRepository::load() {
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
            !parseInteger(semester, record.semester) ||
            !parseInteger(marks, record.marks)) {
            records_.clear();
            return false;
        }
        upsert(std::move(record));
    }
    return true;
}

bool GradeRepository::save() const {
    std::ofstream output(filePath_, std::ios::trunc);
    if (!output) {
        return false;
    }
    for (const MarkRecord& record : records_) {
        output << record.rollNumber << '\t' << record.subject << '\t'
               << record.semester << '\t' << record.marks << '\n';
    }
    return output.good();
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
    if (!session.authenticated || session.role != Role::Teacher) {
        return failure(ErrorCode::AccessDenied,
                       "Only an authenticated teacher may enter marks.");
    }
    if (!storageReady_) {
        return failure(ErrorCode::StorageFailure, "Grade data could not be loaded.");
    }
    if (!validField(rollNumber) || !validField(subject) || semester <= 0) {
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
    if (!parseInteger(marksInput, marks) || marks < 0 || marks > 100) {
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

ReportResult GradeService::generateReportCard(const Session& session,
                                              const std::string& rollNumber) const {
    if (!session.authenticated) {
        return {false, ErrorCode::AccessDenied, "Authentication is required.", {}};
    }
    if (session.role == Role::Student && session.studentRollNumber != rollNumber) {
        return {false, ErrorCode::AccessDenied,
                "Students may view only their own report card.", {}};
    }
    if (!studentExists_ || !studentExists_(rollNumber)) {
        return {false, ErrorCode::StudentNotFound, "Student was not found.", {}};
    }

    std::ostringstream report;
    report << "Report card for " << rollNumber << '\n';
    for (const MarkRecord& record : repository_.records()) {
        if (record.rollNumber == rollNumber) {
            report << record.subject << " | Semester " << record.semester
                   << " | Marks " << record.marks << '\n';
        }
    }
    return {true, ErrorCode::None, "Report generated.", report.str()};
}

}  // namespace srm
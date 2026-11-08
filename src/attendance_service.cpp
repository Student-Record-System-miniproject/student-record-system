#include "srm/attendance_service.h"
#include "srm/report_export.h"
#include "srm/rbac.h"
#include "srm/storage_utils.h"
#include "srm/input_validator.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <utility>
#include <ctime>

namespace srm {
namespace {

bool parseStatus(const std::string& value, AttendanceStatus& status) {
    if (value == "P") {
        status = AttendanceStatus::Present;
        return true;
    }
    if (value == "A") {
        status = AttendanceStatus::Absent;
        return true;
    }
    return false;
}

bool isValidCalendarDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }

    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }

        if (date[i] < '0' || date[i] > '9') {
            return false;
        }
    }

    const int year =
        (date[0] - '0') * 1000 +
        (date[1] - '0') * 100 +
        (date[2] - '0') * 10 +
        (date[3] - '0');

    const int month =
        (date[5] - '0') * 10 + (date[6] - '0');

    const int day =
        (date[8] - '0') * 10 + (date[9] - '0');

    if (year < 1 || month < 1 || month > 12) {
        return false;
    }

    const bool leapYear =
        (year % 400 == 0) ||
        (year % 4 == 0 && year % 100 != 0);

    const int daysInMonth[] = {
        31, leapYear ? 29 : 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    return day >= 1 && day <= daysInMonth[month - 1];
}

std::string currentLocalDate() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};

#if defined(_WIN32)
    if (localtime_s(&localTime, &now) != 0) {
        return {};
    }
#else
    if (localtime_r(&now, &localTime) == nullptr) {
        return {};
    }
#endif

    std::ostringstream date;
    date << std::put_time(&localTime, "%Y-%m-%d");

    return date.str();
}

}  // namespace

AttendanceRepository::AttendanceRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool AttendanceRepository::load() {
    recoverInterruptedSave(filePath_);
    records_.clear();
    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        AttendanceRecord record;
        std::string status;
        if (!std::getline(row, record.rollNumber, '\t') ||
            !std::getline(row, record.subject, '\t') ||
            !std::getline(row, record.date, '\t') ||
            !std::getline(row, status) ||
            !InputValidator::validateText(record.rollNumber) ||
            !InputValidator::validateText(record.subject) ||
            !InputValidator::validateText(record.date) ||
            !parseStatus(status, record.status)) {
            records_.clear();
            return false;
        }
        records_.push_back(std::move(record));
    }
    return true;
}

bool AttendanceRepository::save() const {
    return atomicWriteFile(filePath_, [&](std::ostream& output) {
        for (const AttendanceRecord& record : records_) {
            output << record.rollNumber << '\t' << record.subject << '\t'
                   << record.date << '\t'
                   << (record.status == AttendanceStatus::Present ? "P" : "A") << '\n';
        }
        return output.good();
    });
}

void AttendanceRepository::upsert(AttendanceRecord record) {
    for (AttendanceRecord& existing : records_) {
        if (existing.rollNumber == record.rollNumber &&
            existing.subject == record.subject &&
            existing.date == record.date) {
            existing.status = record.status;
            return;
        }
    }

    records_.push_back(std::move(record));
}

std::vector<AttendanceRecord> AttendanceRepository::snapshot() const {
    return records_;
}

void AttendanceRepository::restore(std::vector<AttendanceRecord> records) {
    records_ = std::move(records);
}

bool AttendanceRepository::removeByRollNumber(const std::string& rollNumber) {
    const auto prevRecords = records_;
    const auto initialSize = records_.size();
    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const AttendanceRecord& r) { return r.rollNumber == rollNumber; }),
        records_.end());
    if (records_.size() != initialSize) {
        if (!save()) {
            records_ = prevRecords;
            return false;
        }
    }
    return true;
}

const std::vector<AttendanceRecord>& AttendanceRepository::records() const {
    return records_;
}

AttendanceService::AttendanceService(std::filesystem::path attendanceFile,
                                     ExistsCheck studentExists)
    : repository_(std::move(attendanceFile)),
      studentExists_(std::move(studentExists)),
      storageReady_(repository_.load()) {}

OperationResult AttendanceService::markAttendance(
    const Session& session,
    const std::string& rollNumber,
    const std::string& subject,
    const std::string& date,
    AttendanceStatus status) {

    // Only teachers are allowed to mark attendance.
    const auto auth = AccessController::authorize(
        session, Operation::MarkAttendance, rollNumber);

    if (!auth.success) {
        return auth;
    }

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure,
                "Attendance data could not be loaded."};
    }

    // Validate student roll number and subject.
    if (!InputValidator::validateText(rollNumber) ||
        !InputValidator::validateText(subject)) {
        return {false, ErrorCode::InvalidInput,
                "Student roll number and subject must be valid."};
    }

    // Check date format and calendar validity.
    if (!isValidCalendarDate(date)) {
        return {false, ErrorCode::InvalidInput,
                "Invalid date. Use YYYY-MM-DD format."};
    }

    // Reject dates later than today.
    const std::string today = currentLocalDate();

    if (today.empty()) {
        return {false, ErrorCode::InvalidInput,
                "Could not determine the current date."};
    }

    if (date > today) {
        return {false, ErrorCode::InvalidInput,
                "Future attendance dates are not allowed."};
    }

    // Check whether the student exists.
    if (!studentExists_ || !studentExists_(rollNumber)) {
        return {false, ErrorCode::StudentNotFound,
                "Student was not found."};
    }

    // Preserve existing records in case saving fails.
    const auto previousRecords = repository_.snapshot();

    repository_.upsert({rollNumber, subject, date, status});

    if (!repository_.save()) {
        repository_.restore(previousRecords);
        storageReady_ = false;

        return {false, ErrorCode::StorageFailure,
                "Attendance could not be saved."};
    }

    const std::string statusText =
        (status == AttendanceStatus::Present) ? "Present" : "Absent";

    return {true, ErrorCode::None,
            "Attendance marked " + statusText + " successfully."};
}

ReportResult AttendanceService::generateReport(
    const Session& session,
    const std::string& rollNumber) const {
    const auto op = (session.role == Role::Student)
                        ? Operation::ViewOwnAttendanceReport
                        : Operation::ViewAnyStudentAttendanceReport;
    const auto auth = AccessController::authorize(session, op, rollNumber);
    if (!auth.success) {
        return {false, auth.error, auth.message, {}};
    }
    if (!InputValidator::validateText(rollNumber)) {
        return {false, ErrorCode::InvalidInput,
                "Invalid student roll number.", {}};
    }

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure,
                "Attendance data could not be loaded.", {}};
    }
    if (!studentExists_ || !studentExists_(rollNumber)) {
        return {false, ErrorCode::StudentNotFound, "Student was not found.", {}};
    }

    std::map<std::string, AttendanceSummary> summaries;
    for (const AttendanceRecord& record : repository_.records()) {
        if (record.rollNumber != rollNumber) {
            continue;
        }
        auto& summary = summaries[record.subject];
        summary.subject = record.subject;
        ++summary.daysHeld;
        if (record.status == AttendanceStatus::Present) {
            ++summary.daysPresent;
        }
    }

    std::ostringstream report;
    report << "Attendance report for " << rollNumber << '\n';
    report << std::fixed << std::setprecision(1);
    for (auto& [subject, summary] : summaries) {
        summary.percentage = summary.daysHeld == 0
                                 ? 0.0
                                 : (100.0 * summary.daysPresent) / summary.daysHeld;
        summary.belowThreshold = summary.percentage < 75.0;
        report << subject << " | Present " << summary.daysPresent << "/"
               << summary.daysHeld << " | " << summary.percentage << "%";
        if (summary.belowThreshold) {
            report << " | BELOW 75%";
        }
        report << '\n';
    }
    return {true, ErrorCode::None, "Attendance report generated.", report.str()};
}

OperationResult AttendanceService::exportReport(
    const Session& session,
    const std::string& rollNumber,
    const std::filesystem::path& outputFile) const {
    return exportTextReport(generateReport(session, rollNumber), outputFile);
}

bool AttendanceService::deleteStudentAttendance(const std::string& rollNumber) {
    if (!InputValidator::validateText(rollNumber)) {
        return false;
    }

    return repository_.removeByRollNumber(rollNumber);
}

}  // namespace srm

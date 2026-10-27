#include "srm/attendance_service.h"
#include "srm/report_export.h"
#include "srm/rbac.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <utility>

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

}  // namespace

AttendanceRepository::AttendanceRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool AttendanceRepository::load() {
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
            !std::getline(row, status) || record.rollNumber.empty() ||
            record.subject.empty() || record.date.empty() ||
            !parseStatus(status, record.status)) {
            records_.clear();
            return false;
        }
        records_.push_back(std::move(record));
    }
    return true;
}

bool AttendanceRepository::save() const {
    std::ofstream output(filePath_, std::ios::trunc);
    if (!output) {
        return false;
    }
    for (const AttendanceRecord& record : records_) {
        output << record.rollNumber << '\t' << record.subject << '\t'
               << record.date << '\t'
               << (record.status == AttendanceStatus::Present ? "P" : "A") << '\n';
    }
    return output.good();
}

bool AttendanceRepository::removeByRollNumber(const std::string& rollNumber) {
    const auto initialSize = records_.size();
    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const AttendanceRecord& r) { return r.rollNumber == rollNumber; }),
        records_.end());
    if (records_.size() != initialSize) {
        return save();
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
    return repository_.removeByRollNumber(rollNumber);
}

}  // namespace srm

#include "srm/attendance_service.h"

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
    if (!session.authenticated) {
        return {false, ErrorCode::AccessDenied, "Authentication is required.", {}};
    }
    if (session.role != Role::Teacher && session.role != Role::Student) {
        return {false, ErrorCode::AccessDenied,
                "Only teachers and students may view attendance reports.", {}};
    }
    if (session.role == Role::Student && session.studentRollNumber != rollNumber) {
        return {false, ErrorCode::AccessDenied,
                "Students may view only their own attendance report.", {}};
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

}  // namespace srm
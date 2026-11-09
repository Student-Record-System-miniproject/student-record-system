#include "srm/attendance_service.h"
#include "srm/report_export.h"
#include "srm/rbac.h"
#include "srm/input_validator.h"

#include <iomanip>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace srm {
namespace {

std::map<std::string, AttendanceSummary> summarizeAttendance(
    const std::vector<AttendanceRecord>& records,
    const std::string& rollNumber) {

    std::map<std::string, AttendanceSummary> summaries;

    for (const AttendanceRecord& record : records) {
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

    return summaries;
}

std::string formatAttendanceReport(
    const std::string& rollNumber,
    std::map<std::string, AttendanceSummary>& summaries) {

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

    return report.str();
}

}  // namespace

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

    if (!InputValidator::validateText(rollNumber)) {
        return {false, ErrorCode::InvalidInput,
                "Invalid student roll number.", {}};
    }

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure,
                "Attendance data could not be loaded.", {}};
    }

    if (!studentExists_ || !studentExists_(rollNumber)) {
        return {false, ErrorCode::StudentNotFound,
                "Student was not found.", {}};
    }

    auto summaries = summarizeAttendance(repository_.records(), rollNumber);
    const std::string report = formatAttendanceReport(rollNumber, summaries);

    return {true, ErrorCode::None, "Attendance report generated.", report};
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

#include "srm/attendance_service.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>

namespace {

using srm::AttendanceService;
using srm::ErrorCode;
using srm::Role;
using srm::Session;
using srm::AttendanceStatus;

const Session teacher{Role::Teacher, true, {}};
const Session ownStudent{Role::Student, true, "S001"};
const Session otherStudent{Role::Student, true, "S002"};

std::filesystem::path writeAttendanceFile() {
    const auto path = std::filesystem::temp_directory_path() / "srm-attendance.tsv";
    std::ofstream output(path, std::ios::trunc);
    output << "S001\tMathematics\t2026-10-01\tP\n";
    output << "S001\tMathematics\t2026-10-02\tP\n";
    output << "S001\tMathematics\t2026-10-03\tA\n";
    output << "S002\tMathematics\t2026-10-01\tA\n";
    output << "S002\tMathematics\t2026-10-02\tA\n";
    output.close();
    return path;
}

AttendanceService serviceFor(const std::filesystem::path& path) {
    return AttendanceService(
        path,
        [](const std::string& rollNumber) {
            return rollNumber == "S001" || rollNumber == "S002";
        });
}

void testTeacherCanViewAnyStudent() {
    const auto path = writeAttendanceFile();
    auto service = serviceFor(path);
    const auto report = service.generateReport(teacher, "S002");
    assert(report.success);
    assert(report.report.find("Attendance report for S002") != std::string::npos);
    assert(report.report.find("100.0%") == std::string::npos);
    assert(report.report.find("BELOW 75%") != std::string::npos);
    std::filesystem::remove(path);
}

void testStudentCanViewOwnReport() {
    const auto path = writeAttendanceFile();
    auto service = serviceFor(path);
    const auto report = service.generateReport(ownStudent, "S001");
    assert(report.success);
    assert(report.report.find("Present 2/3") != std::string::npos);
    assert(report.report.find("66.7%") != std::string::npos);
    assert(report.report.find("BELOW 75%") != std::string::npos);
    std::filesystem::remove(path);
}

void testStudentCannotViewAnotherReport() {
    const auto path = writeAttendanceFile();
    auto service = serviceFor(path);
    const auto report = service.generateReport(otherStudent, "S001");
    assert(!report.success);
    assert(report.error == ErrorCode::AccessDenied);
    std::filesystem::remove(path);
}

void testSecureInputValidation() {
    const auto path = writeAttendanceFile();
    auto service = serviceFor(path);

    const std::string longInput(101, 'A');

    // Empty roll number is rejected.
    auto emptyReport = service.generateReport(teacher, "");
    assert(!emptyReport.success);
    assert(emptyReport.error == ErrorCode::InvalidInput);

    // Excessively long roll number is rejected.
    auto longReport = service.generateReport(teacher, longInput);
    assert(!longReport.success);
    assert(longReport.error == ErrorCode::InvalidInput);

    // Newline in roll number is rejected.
    auto malformedReport = service.generateReport(teacher, "S001\n");
    assert(!malformedReport.success);
    assert(malformedReport.error == ErrorCode::InvalidInput);

    // Invalid roll numbers are rejected during deletion.
    assert(!service.deleteStudentAttendance(""));
    assert(!service.deleteStudentAttendance(longInput));
    assert(!service.deleteStudentAttendance("S001\n"));

    // Existing attendance records must remain unchanged.
    const auto report = service.generateReport(teacher, "S001");
    assert(report.success);
    assert(report.report.find("Present 2/3") != std::string::npos);

    std::filesystem::remove(path);
}

void testTeacherCanMarkPresentAndAbsent() {
    const auto path = std::filesystem::temp_directory_path() /
                      "srm-attendance-marking.tsv";
    std::filesystem::remove(path);

    auto service = serviceFor(path);

    // Mark student Present on the first date.
    const auto presentResult = service.markAttendance(
        teacher, "S001", "Mathematics", "2020-01-01",
        AttendanceStatus::Present);

    assert(presentResult.success);

    auto report = service.generateReport(ownStudent, "S001");

    assert(report.success);
    assert(report.report.find("Present 1/1") != std::string::npos);
    assert(report.report.find("100.0%") != std::string::npos);

    // Change the same date to Absent.
    const auto absentResult = service.markAttendance(
        teacher, "S001", "Mathematics", "2020-01-01",
        AttendanceStatus::Absent);

    assert(absentResult.success);

    report = service.generateReport(ownStudent, "S001");

    // Verify that the existing record was updated.
    assert(report.success);
    assert(report.report.find("Present 0/1") != std::string::npos);
    assert(report.report.find("0.0%") != std::string::npos);

    // Mark attendance for a different date.
    const auto secondDateResult = service.markAttendance(
        teacher, "S001", "Mathematics", "2020-01-02",
        AttendanceStatus::Present);

    assert(secondDateResult.success);

    // Verify that both dates are counted separately.
    report = service.generateReport(ownStudent, "S001");

    assert(report.success);
    assert(report.report.find("Present 1/2") != std::string::npos);
    assert(report.report.find("50.0%") != std::string::npos);

    // Verify that both records are stored in the file.
    std::ifstream input(path);

    const std::string savedData(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());

    assert(savedData.find("S001\tMathematics\t2020-01-01\tA") !=
           std::string::npos);

    assert(savedData.find("S001\tMathematics\t2020-01-02\tP") !=
           std::string::npos);

    input.close();
    std::filesystem::remove(path);
}

void testFutureAttendanceDateIsRejected() {
    const auto path = std::filesystem::temp_directory_path() /
                      "srm-attendance-future.tsv";
    std::filesystem::remove(path);

    auto service = serviceFor(path);

    const auto result = service.markAttendance(
        teacher, "S001", "Mathematics", "2999-12-31",
        AttendanceStatus::Present);

    assert(!result.success);
    assert(result.error == ErrorCode::InvalidInput);

    std::filesystem::remove(path);
}

void testInvalidAttendanceDatesAreRejected() {
    const auto path = std::filesystem::temp_directory_path() /
                      "srm-attendance-invalid-date.tsv";
    std::filesystem::remove(path);

    auto service = serviceFor(path);

    const std::vector<std::string> invalidDates = {
        "2026-13-01",
        "2026-02-29",
        "2026-04-31",
        "2026-1-01",
        "not-a-date",
        "2026-00-10"
    };

    for (const std::string& date : invalidDates) {
        const auto result = service.markAttendance(
            teacher, "S001", "Mathematics", date,
            AttendanceStatus::Present);

        assert(!result.success);
        assert(result.error == ErrorCode::InvalidInput);
    }

    std::filesystem::remove(path);
}

void testAttendanceMarkingPermissionsAndStudentValidation() {
    const auto path = std::filesystem::temp_directory_path() /
                      "srm-attendance-permissions.tsv";
    std::filesystem::remove(path);

    auto service = serviceFor(path);

    // Students cannot mark attendance.
    const auto studentResult = service.markAttendance(
        ownStudent, "S001", "Mathematics", "2020-01-01",
        AttendanceStatus::Present);

    assert(!studentResult.success);
    assert(studentResult.error == ErrorCode::AccessDenied);

    // Teachers cannot mark attendance for nonexistent students.
    const auto invalidStudentResult = service.markAttendance(
        teacher, "S999", "Mathematics", "2020-01-01",
        AttendanceStatus::Present);

    assert(!invalidStudentResult.success);
    assert(invalidStudentResult.error == ErrorCode::StudentNotFound);

    std::filesystem::remove(path);
}

}  // namespace

int main() {
    testTeacherCanViewAnyStudent();
    testStudentCanViewOwnReport();
    testStudentCannotViewAnotherReport();
    testSecureInputValidation();
    testTeacherCanMarkPresentAndAbsent();
    testFutureAttendanceDateIsRejected();
    testInvalidAttendanceDatesAreRejected();
    testAttendanceMarkingPermissionsAndStudentValidation();
    return 0;
}
#include "srm/attendance_service.h"
#include "srm/grade_service.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

using srm::AttendanceService;
using srm::ErrorCode;
using srm::GradeService;
using srm::Role;
using srm::Session;

const Session teacher{Role::Teacher, true, {}};

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void testReportCardExportMatchesDisplayedReport() {
    const auto dataPath = std::filesystem::temp_directory_path() / "srm-export-grades.tsv";
    const auto outputPath = std::filesystem::temp_directory_path() / "srm-report-card.txt";
    std::filesystem::remove(dataPath);
    std::filesystem::remove(outputPath);
    GradeService service(
        dataPath,
        [](const std::string& rollNumber) { return rollNumber == "S001"; },
        [](const std::string& subject) { return subject == "Mathematics"; });
    assert(service.enterOrUpdateMarks(teacher, "S001", "Mathematics", 1, "88").success);
    const auto displayed = service.generateReportCard(teacher, "S001");
    assert(displayed.success);
    assert(service.exportReportCard(teacher, "S001", outputPath).success);
    assert(std::filesystem::exists(outputPath));
    assert(readFile(outputPath) == displayed.report);
    std::filesystem::remove(dataPath);
    std::filesystem::remove(outputPath);
}

void testAttendanceExportMatchesDisplayedReport() {
    const auto dataPath = std::filesystem::temp_directory_path() / "srm-export-attendance.tsv";
    const auto outputPath = std::filesystem::temp_directory_path() / "srm-attendance-report.txt";
    {
        std::ofstream output(dataPath, std::ios::trunc);
        output << "S001\tMathematics\t2026-10-01\tP\n";
        output << "S001\tMathematics\t2026-10-02\tA\n";
    }
    std::filesystem::remove(outputPath);
    AttendanceService service(
        dataPath,
        [](const std::string& rollNumber) { return rollNumber == "S001"; });
    const auto displayed = service.generateReport(teacher, "S001");
    assert(displayed.success);
    assert(service.exportReport(teacher, "S001", outputPath).success);
    assert(std::filesystem::exists(outputPath));
    assert(readFile(outputPath) == displayed.report);
    std::filesystem::remove(dataPath);
    std::filesystem::remove(outputPath);
}

void testExportHandlesAuthorizationAndWriteErrors() {
    const auto dataPath = std::filesystem::temp_directory_path() / "srm-export-errors.tsv";
    const auto outputDirectory = std::filesystem::temp_directory_path() / "srm-export-dir";
    std::filesystem::remove(dataPath);
    std::filesystem::remove_all(outputDirectory);
    std::filesystem::create_directory(outputDirectory);
    GradeService service(
        dataPath,
        [](const std::string& rollNumber) { return rollNumber == "S001"; },
        [](const std::string& subject) { return subject == "Mathematics"; });
    const Session student{Role::Student, true, "S001"};
    assert(service.exportReportCard(student, "S002", outputDirectory).error ==
           ErrorCode::AccessDenied);
    assert(service.exportReportCard(teacher, "S001", outputDirectory).error ==
           ErrorCode::StorageFailure);
    std::filesystem::remove_all(outputDirectory);
    std::filesystem::remove(dataPath);
}

}  // namespace

int main() {
    testReportCardExportMatchesDisplayedReport();
    testAttendanceExportMatchesDisplayedReport();
    testExportHandlesAuthorizationAndWriteErrors();
    return 0;
}
#include "srm/grade_service.h"

#include <cassert>
#include <filesystem>
#include <string>

namespace {

using srm::ErrorCode;
using srm::GradeService;
using srm::Role;
using srm::Session;

const Session teacher{Role::Teacher, true, {}};
const Session student{Role::Student, true, "S001"};

GradeService serviceFor(const std::filesystem::path& path) {
    return GradeService(
        path,
        [](const std::string& rollNumber) { return rollNumber == "S001"; },
        [](const std::string& subject) { return subject == "Mathematics"; });
}

void testBoundaryMarksAreAccepted() {
    const auto path = std::filesystem::temp_directory_path() / "srm-grade-boundary.tsv";
    std::filesystem::remove(path);
    auto service = serviceFor(path);
    assert(service.enterOrUpdateMarks(teacher, "S001", "Mathematics", 1, "0").success);
    assert(service.enterOrUpdateMarks(teacher, "S001", "Mathematics", 2, "100").success);
    std::filesystem::remove(path);
}

void testInvalidMarksAreRejected() {
    const auto path = std::filesystem::temp_directory_path() / "srm-grade-invalid.tsv";
    std::filesystem::remove(path);
    auto service = serviceFor(path);
    for (const std::string input : {"-1", "101", "abc"}) {
        const auto result = service.enterOrUpdateMarks(
            teacher, "S001", "Mathematics", 1, input);
        assert(!result.success);
        assert(result.error == ErrorCode::InvalidInput);
    }
    std::filesystem::remove(path);
}

void testUpdateReplacesAndPersists() {
    const auto path = std::filesystem::temp_directory_path() / "srm-grade-update.tsv";
    std::filesystem::remove(path);
    {
        auto service = serviceFor(path);
        assert(service.enterOrUpdateMarks(teacher, "S001", "Mathematics", 1, "72").success);
        assert(service.enterOrUpdateMarks(teacher, "S001", "Mathematics", 1, "88").success);
        const auto report = service.generateReportCard(teacher, "S001");
        assert(report.success);
        assert(report.report.find("Marks 88") != std::string::npos);
        assert(report.report.find("Marks 72") == std::string::npos);
    }
    {
        auto reloaded = serviceFor(path);
        const auto report = reloaded.generateReportCard(teacher, "S001");
        assert(report.success);
        assert(report.report.find("Marks 88") != std::string::npos);
        assert(report.report.find("Marks 72") == std::string::npos);
    }
    std::filesystem::remove(path);
}

void testPermissionsAndIdentifiersAreValidated() {
    const auto path = std::filesystem::temp_directory_path() / "srm-grade-security.tsv";
    std::filesystem::remove(path);
    auto service = serviceFor(path);
    assert(service.enterOrUpdateMarks(student, "S001", "Mathematics", 1, "80").error ==
           ErrorCode::AccessDenied);
    assert(service.enterOrUpdateMarks(teacher, "S999", "Mathematics", 1, "80").error ==
           ErrorCode::StudentNotFound);
    assert(service.enterOrUpdateMarks(teacher, "S001", "Physics", 1, "80").error ==
           ErrorCode::SubjectNotFound);
    std::filesystem::remove(path);
}

}  // namespace

int main() {
    testBoundaryMarksAreAccepted();
    testInvalidMarksAreRejected();
    testUpdateReplacesAndPersists();
    testPermissionsAndIdentifiersAreValidated();
    return 0;
}
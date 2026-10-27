#include "srm/student_service.h"
#include "srm/grade_service.h"
#include "srm/attendance_service.h"

#include <cassert>
#include <filesystem>
#include <string>

namespace {

using srm::AttendanceService;
using srm::ErrorCode;
using srm::GradeService;
using srm::Role;
using srm::Session;
using srm::StudentRecord;
using srm::StudentService;

const Session adminSession{Role::Administrator, true, {}};
const Session teacherSession{Role::Teacher, true, {}};
const Session studentSession{Role::Student, true, "S101"};

void testUpdateStudentFields() {
    const auto path = std::filesystem::temp_directory_path() / "srm-student-update.tsv";
    std::filesystem::remove(path);

    StudentService service(path);
    assert(service.addStudent(adminSession, {"S101", "Alice Smith", "Computer Science", "alice@test.edu"}).success);

    // SRM-F-012: Administrator updates fields
    const auto updateResult = service.updateStudent(
        adminSession, "S101", "Alice Johnson", "Information Technology", "alice.j@test.edu");
    assert(updateResult.success);
    assert(updateResult.error == ErrorCode::None);

    // Verify changed values appear in subsequent searches/queries
    const auto* record = service.getStudent(adminSession, "S101");
    assert(record != nullptr);
    assert(record->rollNumber == "S101");  // Roll number is protected
    assert(record->name == "Alice Johnson");
    assert(record->department == "Information Technology");
    assert(record->email == "alice.j@test.edu");

    // Search by partial name finds the updated name
    const auto searchMatches = service.searchStudents(adminSession, "johnson");
    assert(searchMatches.size() == 1);
    assert(searchMatches[0].rollNumber == "S101");

    // Updating a non-existent student returns StudentNotFound
    const auto notFoundResult = service.updateStudent(
        adminSession, "S999", "Ghost", "Unknown", "ghost@test.edu");
    assert(!notFoundResult.success);
    assert(notFoundResult.error == ErrorCode::StudentNotFound);

    std::filesystem::remove(path);
}

void testUpdateRoleRestrictions() {
    const auto path = std::filesystem::temp_directory_path() / "srm-student-update-sec.tsv";
    std::filesystem::remove(path);

    StudentService service(path);
    service.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu"});

    // Teacher cannot update student records
    const auto teacherResult = service.updateStudent(
        teacherSession, "S101", "Hacked", "Hacked", "hacked@test.edu");
    assert(!teacherResult.success);
    assert(teacherResult.error == ErrorCode::AccessDenied);

    // Student cannot update student records
    const auto studentResult = service.updateStudent(
        studentSession, "S101", "Hacked", "Hacked", "hacked@test.edu");
    assert(!studentResult.success);
    assert(studentResult.error == ErrorCode::AccessDenied);

    std::filesystem::remove(path);
}

void testDeleteRequiresConfirmationAndCanCancel() {
    const auto path = std::filesystem::temp_directory_path() / "srm-student-del-cancel.tsv";
    std::filesystem::remove(path);

    bool cascadeTriggered = false;
    StudentService service(path, [&](const std::string&) { cascadeTriggered = true; });

    service.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu"});

    // Explicit confirmation "N" keeps the record
    const auto cancelResult = service.deleteStudent(adminSession, "S101", "N");
    assert(!cancelResult.success);
    assert(!cascadeTriggered);
    assert(service.getStudent(adminSession, "S101") != nullptr);

    // Any answer other than Y/y keeps the record
    const auto randomResult = service.deleteStudent(adminSession, "S101", "No");
    assert(!randomResult.success);
    assert(!cascadeTriggered);
    assert(service.getStudent(adminSession, "S101") != nullptr);

    std::filesystem::remove(path);
}

void testDeleteWithConfirmationRemovesStudentAndCascades() {
    const auto studentPath = std::filesystem::temp_directory_path() / "srm-cascade-student.tsv";
    const auto gradePath = std::filesystem::temp_directory_path() / "srm-cascade-grade.tsv";
    const auto attPath = std::filesystem::temp_directory_path() / "srm-cascade-att.tsv";

    std::filesystem::remove(studentPath);
    std::filesystem::remove(gradePath);
    std::filesystem::remove(attPath);

    GradeService gradeService(
        gradePath,
        [](const std::string&) { return true; },
        [](const std::string&) { return true; });

    AttendanceService attService(
        attPath,
        [](const std::string&) { return true; });

    // Enter initial grade and attendance for S101
    assert(gradeService.enterOrUpdateMarks(teacherSession, "S101", "Math", 1, "95").success);
    assert(gradeService.generateReportCard(teacherSession, "S101").report.find("Marks 95") != std::string::npos);

    StudentService studentService(studentPath, [&](const std::string& roll) {
        gradeService.deleteStudentGrades(roll);
        attService.deleteStudentAttendance(roll);
    });

    studentService.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu"});
    assert(studentService.getStudent(adminSession, "S101") != nullptr);

    // SRM-F-013: Answering "Y" removes the student, grades, and attendance
    const auto deleteResult = studentService.deleteStudent(adminSession, "S101", "Y");
    assert(deleteResult.success);
    assert(deleteResult.error == ErrorCode::None);

    // Student record is removed
    assert(studentService.getStudent(adminSession, "S101") == nullptr);
    assert(!studentService.exists("S101"));

    // Grades for S101 are deleted via cascade
    const auto reportAfter = gradeService.generateReportCard(teacherSession, "S101");
    assert(reportAfter.report.find("Marks 95") == std::string::npos);

    // Deleting already deleted student returns StudentNotFound
    const auto repeatDelete = studentService.deleteStudent(adminSession, "S101", "Y");
    assert(!repeatDelete.success);
    assert(repeatDelete.error == ErrorCode::StudentNotFound);

    std::filesystem::remove(studentPath);
    std::filesystem::remove(gradePath);
    std::filesystem::remove(attPath);
}

void testDeleteRoleRestrictions() {
    const auto path = std::filesystem::temp_directory_path() / "srm-student-del-sec.tsv";
    std::filesystem::remove(path);

    StudentService service(path);
    service.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu"});

    // Teacher cannot delete
    const auto teacherResult = service.deleteStudent(teacherSession, "S101", "Y");
    assert(!teacherResult.success);
    assert(teacherResult.error == ErrorCode::AccessDenied);

    // Student cannot delete
    const auto studentResult = service.deleteStudent(studentSession, "S101", "Y");
    assert(!studentResult.success);
    assert(studentResult.error == ErrorCode::AccessDenied);

    // Record is still preserved
    assert(service.getStudent(adminSession, "S101") != nullptr);

    std::filesystem::remove(path);
}

}  // namespace

int main() {
    testUpdateStudentFields();
    testUpdateRoleRestrictions();
    testDeleteRequiresConfirmationAndCanCancel();
    testDeleteWithConfirmationRemovesStudentAndCascades();
    testDeleteRoleRestrictions();
    return 0;
}

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
    assert(service.addStudent(adminSession, {"S101", "Alice Smith", "Computer Science", "alice@test.edu", "2005-04-12", 2, "9876543210"}).success);

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
    service.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu", "2005-04-12", 2, "9876543210"});

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
    StudentService service(path, [&](const std::string&) {
        cascadeTriggered = true;
        return true;
    });

    service.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu", "2005-04-12", 2, "9876543210"});

    // Explicit confirmation "N" keeps the record
    const auto cancelResult = service.deleteStudent(adminSession, "S101", "N");
    assert(!cancelResult.success);
    assert(!cascadeTriggered);
    assert(service.getStudent(adminSession, "S101") != nullptr);

    // Invalid confirmation such as "No" is rejected.
    const auto randomResult = service.deleteStudent(adminSession, "S101", "No");
    assert(randomResult.error == ErrorCode::InvalidInput);
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
        bool gOk = gradeService.deleteStudentGrades(roll);
        bool aOk = attService.deleteStudentAttendance(roll);
        return gOk && aOk;
    });

    studentService.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu", "2005-04-12", 2, "9876543210"});
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
    service.addStudent(adminSession, {"S101", "Alice Smith", "CSE", "alice@test.edu", "2005-04-12", 2, "9876543210"});

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

void testAddStudentRbacAndDuplicates() {
    const auto path = std::filesystem::temp_directory_path() / "srm-student-add-dup.tsv";
    std::filesystem::remove(path);

    StudentService service(path);

    // Teacher cannot add students (RBAC Operation::AddStudent)
    const auto teacherAdd = service.addStudent(teacherSession, {"S201", "Bob", "ECE", "bob@test.edu", "2005-04-12", 2, "9876543210"});
    assert(!teacherAdd.success);
    assert(teacherAdd.error == ErrorCode::AccessDenied);

    // Student cannot add students
    const auto studentAdd = service.addStudent(studentSession, {"S201", "Bob", "ECE", "bob@test.edu", "2005-04-12", 2, "9876543210"});
    assert(!studentAdd.success);
    assert(studentAdd.error == ErrorCode::AccessDenied);

    // Administrator adds student successfully
    const auto adminAdd = service.addStudent(adminSession, {"S201", "Bob", "ECE", "bob@test.edu", "2005-04-12", 2, "9876543210"});
    assert(adminAdd.success);
    assert(adminAdd.error == ErrorCode::None);

    // Adding duplicate roll number returns ErrorCode::DuplicateRecord
    const auto dupAdd = service.addStudent(
        adminSession,
        {"S201", "Robert", "ME", "robert@test.edu",
         "2004-08-20", 3, "9123456780"});

    assert(!dupAdd.success);
    assert(dupAdd.error == ErrorCode::DuplicateRecord);

    // Original student record must remain unchanged.
    const auto* original = service.getStudent(adminSession, "S201");

    assert(original != nullptr);
    assert(original->rollNumber == "S201");
    assert(original->name == "Bob");
    assert(original->department == "ECE");
    assert(original->email == "bob@test.edu");
    assert(original->dateOfBirth == "2005-04-12");
    assert(original->year == 2);
    assert(original->contactNumber == "9876543210");

    std::filesystem::remove(path);
}

void testCascadeFailureRollsBackStudentDeletion() {
    const auto path = std::filesystem::temp_directory_path() / "srm-student-cascade-fail.tsv";
    std::filesystem::remove(path);

    // Set up cascade hook that simulates storage failure
    StudentService service(path, [](const std::string&) {
        return false;  // Failure during cascade
    });

    assert(service.addStudent(adminSession, {"S301", "Charlie", "ME", "charlie@test.edu", "2005-04-12", 2, "9876543210"}).success);
    assert(service.exists("S301"));

    // Attempt delete with confirmation: cascade hook returns false
    const auto deleteResult = service.deleteStudent(adminSession, "S301", "Y");
    assert(!deleteResult.success);
    assert(deleteResult.error == ErrorCode::StorageFailure);

    // Transactional consistency: Student S301 must NOT be deleted (rolled back)
    assert(service.exists("S301"));
    const auto* record = service.getStudent(adminSession, "S301");
    assert(record != nullptr);
    assert(record->name == "Charlie");

    std::filesystem::remove(path);
}

void testSecureInputValidation() {
    const auto path = std::filesystem::temp_directory_path() /
                      "srm-student-input-validation.tsv";
    std::filesystem::remove(path);

    StudentService service(path);

    // Create one valid student for lookup and update tests.
    assert(service.addStudent(
        adminSession,
        {"S101", "Alice Smith", "CSE", "alice@test.edu", "2005-04-12", 2, "9876543210"}).success);

    const std::string longInput(101, 'A');

    // 1. Reject empty student fields.
    const auto emptyField = service.addStudent(
        adminSession, {"", "Bob", "ECE", "bob@test.edu", "2005-04-12", 2, "9876543210"});
    assert(!emptyField.success);
    assert(emptyField.error == ErrorCode::InvalidInput);

    // 2. Reject fields exceeding the maximum length.
    const auto longField = service.addStudent(
        adminSession, {"S102", longInput, "ECE", "bob@test.edu", "2005-04-12", 2, "9876543210"});
    assert(!longField.success);
    assert(longField.error == ErrorCode::InvalidInput);

    // 3. Reject control characters in student fields.
    const auto malformedField = service.addStudent(
        adminSession, {"S103", "Alice\nSmith", "CSE", "alice@test.edu", "2005-04-12", 2, "9876543210"});
    assert(!malformedField.success);
    assert(malformedField.error == ErrorCode::InvalidInput);

    // 4. Reject invalid date of birth.
    const auto invalidDob = service.addStudent(
        adminSession,
        {"S104", "Bob", "ECE", "bob@test.edu",
         "2005-02-29", 2, "9876543210"});
    assert(!invalidDob.success);
    assert(invalidDob.error == ErrorCode::InvalidInput);

    // 5. Reject invalid academic year.
    const auto invalidYear = service.addStudent(
        adminSession,
        {"S105", "Bob", "ECE", "bob@test.edu",
         "2005-04-12", 5, "9876543210"});
    assert(!invalidYear.success);
    assert(invalidYear.error == ErrorCode::InvalidInput);

    // 6. Reject invalid contact number.
    const auto invalidContact = service.addStudent(
        adminSession,
        {"S106", "Bob", "ECE", "bob@test.edu",
         "2005-04-12", 2, "98765AB210"});
    assert(!invalidContact.success);
    assert(invalidContact.error == ErrorCode::InvalidInput);

    // 7. Reject invalid input while updating a student.
    const auto invalidUpdate = service.updateStudent(
        adminSession, "S101", longInput, "CSE", "alice@test.edu");
    assert(!invalidUpdate.success);
    assert(invalidUpdate.error == ErrorCode::InvalidInput);

    // 8. Reject malformed roll numbers during deletion.
    const auto invalidRoll = service.deleteStudent(
        adminSession, "S101\n", "Y");
    assert(!invalidRoll.success);
    assert(invalidRoll.error == ErrorCode::InvalidInput);

    // 9. Reject invalid deletion confirmations.
    const auto invalidConfirmation = service.deleteStudent(
        adminSession, "S101", "Yes");
    assert(!invalidConfirmation.success);
    assert(invalidConfirmation.error == ErrorCode::InvalidInput);

    // Student record must remain after rejected deletion attempts.
    assert(service.exists("S101"));

    // 10. Reject empty and excessively long search queries.
    assert(service.searchStudents(adminSession, "").empty());
    assert(service.searchStudents(adminSession, longInput).empty());

    // 11. Reject malformed lookup inputs.
    assert(service.getStudent(adminSession, "S101\n") == nullptr);
    assert(!service.exists("S101\n"));

    std::filesystem::remove(path);
}

void testStudentRecordPersistence() {
    const auto path = std::filesystem::temp_directory_path() /
                      "srm-student-persistence.tsv";
    std::filesystem::remove(path);

    {
        StudentService service(path);

        const auto result = service.addStudent(
            adminSession,
            {"S401", "David", "CSE", "david@test.edu",
             "2004-06-15", 3, "9988776655"});

        assert(result.success);
    }

    // Create a new service instance to reload the saved record.
    {
        StudentService service(path);

        const auto* record = service.getStudent(adminSession, "S401");

        assert(record != nullptr);
        assert(record->rollNumber == "S401");
        assert(record->name == "David");
        assert(record->department == "CSE");
        assert(record->email == "david@test.edu");
        assert(record->dateOfBirth == "2004-06-15");
        assert(record->year == 3);
        assert(record->contactNumber == "9988776655");
    }

    std::filesystem::remove(path);
}

}  // namespace

int main() {
    testUpdateStudentFields();
    testUpdateRoleRestrictions();
    testDeleteRequiresConfirmationAndCanCancel();
    testDeleteWithConfirmationRemovesStudentAndCascades();
    testDeleteRoleRestrictions();
    testAddStudentRbacAndDuplicates();
    testCascadeFailureRollsBackStudentDeletion();
    testSecureInputValidation();
    testStudentRecordPersistence();
    return 0;
}

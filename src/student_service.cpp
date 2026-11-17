#include "srm/student_service.h"
#include "srm/audit_service.h"
#include "srm/storage_utils.h"
#include "srm/input_validator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {
namespace {

OperationResult completeStudentDeletion(
    StudentRepository& repository,
    const StudentService::CascadeDeleteHook& cascadeDelete,
    AuditService* auditService,
    bool requireAudit,
    const Session& session,
    const std::string& rollNumber,
    const StudentRecord& studentSnapshot) {

    if (cascadeDelete && !cascadeDelete(rollNumber)) {
        repository.add(studentSnapshot);

        return {false, ErrorCode::StorageFailure,
                "Cascade deletion of associated records failed. Operation rolled back."};
    }

    if (auditService) {
        const bool logged = auditService->logOperation(
            session, "DELETE_STUDENT", rollNumber, true,
            "Student record and associated records deleted");

        if (!logged && requireAudit) {
            repository.add(studentSnapshot);

            return {false, ErrorCode::StorageFailure,
                    "Operation aborted: Audit logging failure (fail-secure policy)"};
        }
    }

    return {true, ErrorCode::None,
            "Student record and associated records deleted."};
}

}  // namespace

StudentService::StudentService(std::filesystem::path studentFile,
                               CascadeDeleteHook cascadeDelete)
    : repository_(std::move(studentFile)),
      cascadeDelete_(std::move(cascadeDelete)),
      storageReady_(repository_.load()) {}

OperationResult StudentService::addStudent(const Session& session,
                                           StudentRecord student) {
    const auto auth = AccessController::authorize(session, Operation::AddStudent);
    if (!auth.success) {
        return auth;
    }
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "Student storage is not ready."};
    }

    if (!InputValidator::validateText(student.rollNumber) ||
        !InputValidator::validateText(student.name) ||
        !InputValidator::validateText(student.department) ||
        !InputValidator::validateText(student.email) ||
        !InputValidator::validateDateOfBirth(student.dateOfBirth) ||
        !InputValidator::validateRange(student.year, 1, 4) ||
        !InputValidator::validateContactNumber(student.contactNumber)) {
        return {false, ErrorCode::InvalidInput,
                "All student fields must be valid. Year must be between 1 and 4."};
    }

    if (repository_.findByRollNumber(student.rollNumber) != nullptr) {
        return {false, ErrorCode::DuplicateRecord, "Student with this roll number already exists."};
    }
    const std::string rollNumber = student.rollNumber;
    const std::string studentName = student.name;
    if (!repository_.add(std::move(student))) {
        return {false, ErrorCode::StorageFailure, "Failed to save student record to storage."};
    }
    if (auditService_) {
        const bool logged = auditService_->logOperation(session, "CREATE_STUDENT", rollNumber, true, "Created student " + studentName);
        if (!logged && requireAudit_) {
            repository_.remove(rollNumber);
            return {false, ErrorCode::StorageFailure, "Operation aborted: Audit logging failure (fail-secure policy)"};
        }
    }
    return {true, ErrorCode::None, "Student record created."};
}

OperationResult StudentService::updateStudent(const Session& session,
                                              const std::string& rollNumber,
                                              const std::string& newName,
                                              const std::string& newDepartment,
                                              const std::string& newEmail) {
    const auto auth = AccessController::authorize(session, Operation::UpdateStudent);
    if (!auth.success) {
        return auth;
    }
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "Student storage is not ready."};
    }

    if (!InputValidator::validateText(rollNumber) ||
        !InputValidator::validateText(newName) ||
        !InputValidator::validateText(newDepartment) ||
        !InputValidator::validateText(newEmail)) {
        return {false, ErrorCode::InvalidInput,
                "Student fields must be non-empty and valid."};
    }

    if (repository_.findByRollNumber(rollNumber) == nullptr) {
        return {false, ErrorCode::StudentNotFound, "Student record not found."};
    }

    if (!repository_.update(rollNumber, newName, newDepartment, newEmail)) {
        return {false, ErrorCode::StorageFailure, "Failed to save updated student record."};
    }
    if (auditService_) {
        const bool logged = auditService_->logOperation(session, "UPDATE_STUDENT", rollNumber, true, "Updated student record");
        if (!logged && requireAudit_) {
            return {false, ErrorCode::StorageFailure, "Operation aborted: Audit logging failure (fail-secure policy)"};
        }
    }
    return {true, ErrorCode::None, "Student record updated successfully."};
}

OperationResult StudentService::deleteStudent(
    const Session& session,
    const std::string& rollNumber,
    const std::string& confirmation) {

    const auto auth =
        AccessController::authorize(session, Operation::DeleteStudent);

    if (!auth.success) {
        return auth;
    }

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure,
                "Student storage is not ready."};
    }

    if (!InputValidator::validateText(rollNumber)) {
        return {false, ErrorCode::InvalidInput,
                "Student roll number contains invalid input."};
    }

    if (!InputValidator::validateConfirmation(confirmation)) {
        return {false, ErrorCode::InvalidInput,
                "Confirmation must be Y or N."};
    }

    if (confirmation == "N" || confirmation == "n") {
        return {false, ErrorCode::None,
                "Deletion cancelled. Student record retained."};
    }

    const auto* existing = repository_.findByRollNumber(rollNumber);

    if (existing == nullptr) {
        return {false, ErrorCode::StudentNotFound,
                "Student record not found."};
    }

    const auto studentSnapshot = *existing;

    if (!repository_.remove(rollNumber)) {
        return {false, ErrorCode::StorageFailure,
                "Failed to delete student record from storage."};
    }

    return completeStudentDeletion(
        repository_, cascadeDelete_, auditService_, requireAudit_,
        session, rollNumber, studentSnapshot);
}

const StudentRecord* StudentService::getStudent(const Session& session,
                                                const std::string& rollNumber) const {
    if (!session.authenticated) {
        return nullptr;
    }

    if (!InputValidator::validateText(rollNumber)) {
        return nullptr;
    }
    if (session.role == Role::Student && session.studentRollNumber != rollNumber) {
        return nullptr;
    }
    return repository_.findByRollNumber(rollNumber);
}

std::vector<StudentRecord> StudentService::searchStudents(const Session& session,
                                                          const std::string& query) const {
    if (!session.authenticated || session.role == Role::Student) {
        return {};
    }
    if (!InputValidator::validateText(query)) {
        return {};
    }
    return repository_.searchByName(query);
}

std::vector<StudentRecord> StudentService::listStudents(
    const Session& session) const {

    // Only authenticated Administrators and Teachers can list students.
    if (!session.authenticated ||
        (session.role != Role::Administrator &&
         session.role != Role::Teacher)) {
        return {};
    }

    if (!storageReady_) {
        return {};
    }

    // Copy all student records.
    std::vector<StudentRecord> students = repository_.records();

    // Sort by ascending roll number.
    std::sort(students.begin(), students.end(),
              [](const StudentRecord& a, const StudentRecord& b) {
                  return a.rollNumber < b.rollNumber;
              });

    return students;
}

bool StudentService::exists(const std::string& rollNumber) const {
    if (!InputValidator::validateText(rollNumber)) {
        return false;
    }

    return repository_.findByRollNumber(rollNumber) != nullptr;
}

void StudentService::setAuditService(AuditService* service, bool requireAudit) {
    auditService_ = service;
    requireAudit_ = requireAudit;
}

AuditService* StudentService::auditService() const {
    return auditService_;
}

bool StudentService::isAuditRequired() const {
    return requireAudit_;
}

}  // namespace srm

#pragma once

#include "srm/common_types.h"
#include "srm/rbac.h"
#include "srm/student_repository.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace srm {

class AuditService;

class StudentService {
public:
    using CascadeDeleteHook = std::function<bool(const std::string& rollNumber)>;

    explicit StudentService(std::filesystem::path studentFile,
                            CascadeDeleteHook cascadeDelete = nullptr);

    // Add a student record (Administrator only)
    OperationResult addStudent(const Session& session,
                               StudentRecord student);

    // SRM-F-012: Update student record
    // - Roll number is immutable (cannot be changed).
    // - Only Administrator may update student records.
    // - Changes appear in subsequent searches.
    OperationResult updateStudent(const Session& session,
                                  const std::string& rollNumber,
                                  const std::string& newName,
                                  const std::string& newDepartment,
                                  const std::string& newEmail);

    // SRM-F-013: Delete student record with explicit confirmation
    // - Only Administrator may delete student records.
    // - Confirmation: "Y" or "y" deletes; "N" or "n" keeps the record.
    // - Deleting student record cascades to delete student's grades and attendance.
    OperationResult deleteStudent(const Session& session,
                                  const std::string& rollNumber,
                                  const std::string& confirmation);

    const StudentRecord* getStudent(const Session& session,
                                    const std::string& rollNumber) const;

    std::vector<StudentRecord> searchStudents(const Session& session,
                                              const std::string& query) const;

    // SRM-F-015: List all students in ascending roll-number order.
    // Only Administrator and Teacher may access this list.
    std::vector<StudentRecord> listStudents(const Session& session) const;

    bool exists(const std::string& rollNumber) const;

    void setAuditService(AuditService* auditService, bool requireAudit = false);
    AuditService* auditService() const;
    bool isAuditRequired() const;

private:
    StudentRepository repository_;
    CascadeDeleteHook cascadeDelete_;
    bool storageReady_;
    AuditService* auditService_ = nullptr;
    bool requireAudit_ = false;
};

}  // namespace srm

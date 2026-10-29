#pragma once

#include "srm/common_types.h"
#include "srm/rbac.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace srm {

struct StudentRecord {
    std::string rollNumber;
    std::string name;
    std::string department;
    std::string email;
};

class StudentRepository {
public:
    explicit StudentRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;
    bool add(StudentRecord student);
    bool update(const std::string& rollNumber,
                const std::string& name,
                const std::string& department,
                const std::string& email);
    bool remove(const std::string& rollNumber);
    const StudentRecord* findByRollNumber(const std::string& rollNumber) const;
    std::vector<StudentRecord> searchByName(const std::string& query) const;
    const std::vector<StudentRecord>& records() const;

private:
    std::filesystem::path filePath_;
    std::vector<StudentRecord> records_;
};

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

    bool exists(const std::string& rollNumber) const;

private:
    StudentRepository repository_;
    CascadeDeleteHook cascadeDelete_;
    bool storageReady_;
};

}  // namespace srm

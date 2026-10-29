#include "srm/student_service.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {
namespace {

bool validField(const std::string& value) {
    return !value.empty() && value.find('\t') == std::string::npos &&
           value.find('\n') == std::string::npos;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

StudentRepository::StudentRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool StudentRepository::load() {
    records_.clear();
    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        StudentRecord student;
        if (!std::getline(row, student.rollNumber, '\t') ||
            !std::getline(row, student.name, '\t') ||
            !std::getline(row, student.department, '\t') ||
            !std::getline(row, student.email) ||
            !validField(student.rollNumber) ||
            !validField(student.name) ||
            !validField(student.department) ||
            !validField(student.email)) {
            records_.clear();
            return false;
        }
        records_.push_back(std::move(student));
    }
    return true;
}

bool StudentRepository::save() const {
    std::ofstream output(filePath_, std::ios::trunc);
    if (!output) {
        return false;
    }
    for (const StudentRecord& student : records_) {
        output << student.rollNumber << '\t' << student.name << '\t'
               << student.department << '\t' << student.email << '\n';
    }
    return output.good();
}

bool StudentRepository::add(StudentRecord student) {
    if (findByRollNumber(student.rollNumber) != nullptr) {
        return false;
    }
    records_.push_back(std::move(student));
    if (!save()) {
        records_.pop_back();
        return false;
    }
    return true;
}

bool StudentRepository::update(const std::string& rollNumber,
                               const std::string& name,
                               const std::string& department,
                               const std::string& email) {
    for (StudentRecord& record : records_) {
        if (record.rollNumber == rollNumber) {
            const auto oldRecord = record;
            record.name = name;
            record.department = department;
            record.email = email;
            if (!save()) {
                record = oldRecord;
                return false;
            }
            return true;
        }
    }
    return false;
}

bool StudentRepository::remove(const std::string& rollNumber) {
    const auto prevRecords = records_;
    const auto initialSize = records_.size();
    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const StudentRecord& r) { return r.rollNumber == rollNumber; }),
        records_.end());
    if (records_.size() != initialSize) {
        if (!save()) {
            records_ = prevRecords;
            return false;
        }
        return true;
    }
    return false;
}

const StudentRecord* StudentRepository::findByRollNumber(const std::string& rollNumber) const {
    for (const StudentRecord& record : records_) {
        if (record.rollNumber == rollNumber) {
            return &record;
        }
    }
    return nullptr;
}

std::vector<StudentRecord> StudentRepository::searchByName(const std::string& query) const {
    std::vector<StudentRecord> matches;
    const std::string queryLower = toLower(query);
    for (const StudentRecord& record : records_) {
        if (toLower(record.name).find(queryLower) != std::string::npos) {
            matches.push_back(record);
        }
    }
    return matches;
}

const std::vector<StudentRecord>& StudentRepository::records() const {
    return records_;
}

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
    if (!validField(student.rollNumber) || !validField(student.name) ||
        !validField(student.department) || !validField(student.email)) {
        return {false, ErrorCode::InvalidInput, "All student fields must be non-empty."};
    }
    if (repository_.findByRollNumber(student.rollNumber) != nullptr) {
        return {false, ErrorCode::DuplicateRecord, "Student with this roll number already exists."};
    }
    if (!repository_.add(std::move(student))) {
        return {false, ErrorCode::StorageFailure, "Failed to save student record to storage."};
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
    if (!validField(rollNumber) || !validField(newName) ||
        !validField(newDepartment) || !validField(newEmail)) {
        return {false, ErrorCode::InvalidInput, "Updated fields must be non-empty and valid."};
    }
    if (repository_.findByRollNumber(rollNumber) == nullptr) {
        return {false, ErrorCode::StudentNotFound, "Student record not found."};
    }

    if (!repository_.update(rollNumber, newName, newDepartment, newEmail)) {
        return {false, ErrorCode::StorageFailure, "Failed to save updated student record."};
    }
    return {true, ErrorCode::None, "Student record updated successfully."};
}

OperationResult StudentService::deleteStudent(const Session& session,
                                              const std::string& rollNumber,
                                              const std::string& confirmation) {
    const auto auth = AccessController::authorize(session, Operation::DeleteStudent);
    if (!auth.success) {
        return auth;
    }
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "Student storage is not ready."};
    }
    const auto* existing = repository_.findByRollNumber(rollNumber);
    if (existing == nullptr) {
        return {false, ErrorCode::StudentNotFound, "Student record not found."};
    }
    const auto studentSnapshot = *existing;

    // Explicit confirmation check (SRM-F-013, TC-STU-03)
    if (confirmation != "Y" && confirmation != "y") {
        return {false, ErrorCode::None, "Deletion cancelled. Student record retained."};
    }

    if (!repository_.remove(rollNumber)) {
        return {false, ErrorCode::StorageFailure, "Failed to delete student record from storage."};
    }

    // Cascade deletion: remove student's grades and attendance
    if (cascadeDelete_) {
        if (!cascadeDelete_(rollNumber)) {
            // Rollback student deletion if cascade fails
            repository_.add(studentSnapshot);
            return {false, ErrorCode::StorageFailure,
                    "Cascade deletion of associated records failed. Operation rolled back."};
        }
    }

    return {true, ErrorCode::None, "Student record and associated records deleted."};
}

const StudentRecord* StudentService::getStudent(const Session& session,
                                                const std::string& rollNumber) const {
    if (!session.authenticated) {
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
    return repository_.searchByName(query);
}

bool StudentService::exists(const std::string& rollNumber) const {
    return repository_.findByRollNumber(rollNumber) != nullptr;
}

}  // namespace srm

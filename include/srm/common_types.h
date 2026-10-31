#pragma once

#include <chrono>
#include <string>

namespace srm {

enum class Role {
    Administrator,
    Teacher,
    Student
};

struct Session {
    Role role = Role::Student;
    bool authenticated = false;
    std::string studentRollNumber = "";
    std::string username = "";
    std::chrono::steady_clock::time_point lastActivity;
};

bool isSessionExpired(const Session& session);

enum class ErrorCode {
    None,
    AccessDenied,
    InvalidInput,
    StudentNotFound,
    SubjectNotFound,
    StorageFailure,
    AccountLocked,
    InvalidCredentials,
    DuplicateRecord
};

struct OperationResult {
    bool success = false;
    ErrorCode error = ErrorCode::None;
    std::string message = "";
};

struct ReportResult {
    bool success = false;
    ErrorCode error = ErrorCode::None;
    std::string message = "";
    std::string report = "";
};

std::string roleToString(Role role);
bool stringToRole(const std::string& str, Role& role);
std::string errorCodeToString(ErrorCode code);

}  // namespace srm

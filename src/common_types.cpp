#include "srm/common_types.h"
#include <chrono>

namespace srm {

std::string roleToString(Role role) {
    switch (role) {
        case Role::Administrator:
            return "Admin";
        case Role::Teacher:
            return "Teacher";
        case Role::Student:
            return "Student";
    }
    return "Student";
}

bool stringToRole(const std::string& str, Role& role) {
    if (str == "Admin" || str == "Administrator") {
        role = Role::Administrator;
        return true;
    }
    if (str == "Teacher") {
        role = Role::Teacher;
        return true;
    }
    if (str == "Student") {
        role = Role::Student;
        return true;
    }
    return false;
}

bool isSessionExpired(const Session& session) {
    if (!session.authenticated) {
        return true;
    }

    const auto elapsed =
        std::chrono::steady_clock::now() - session.lastActivity;

    return elapsed >= std::chrono::minutes(5);
}

std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:
            return "None";
        case ErrorCode::AccessDenied:
            return "AccessDenied";
        case ErrorCode::InvalidInput:
            return "InvalidInput";
        case ErrorCode::StudentNotFound:
            return "StudentNotFound";
        case ErrorCode::SubjectNotFound:
            return "SubjectNotFound";
        case ErrorCode::StorageFailure:
            return "StorageFailure";
        case ErrorCode::AccountLocked:
            return "AccountLocked";
        case ErrorCode::InvalidCredentials:
            return "InvalidCredentials";
        case ErrorCode::DuplicateRecord:
            return "DuplicateRecord";
    }
    return "Unknown";
}

}  // namespace srm

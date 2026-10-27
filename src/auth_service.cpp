#include "srm/auth_service.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {
namespace {

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
    if (str == "Admin") {
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

bool parseInteger(const std::string& input, int& value) {
    if (input.empty()) {
        return false;
    }
    const char* first = input.data();
    const char* last = first + input.size();
    const auto parsed = std::from_chars(first, last, value);
    return parsed.ec == std::errc{} && parsed.ptr == last;
}

bool validField(const std::string& value) {
    return !value.empty() && value.find('\t') == std::string::npos &&
           value.find('\n') == std::string::npos;
}

}  // namespace

UserRepository::UserRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool UserRepository::load() {
    users_.clear();
    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        UserAccount user;
        std::string roleStr;
        std::string failedAttemptsStr;
        std::string isLockedStr;
        if (!std::getline(row, user.username, '\t') ||
            !std::getline(row, user.password, '\t') ||
            !std::getline(row, roleStr, '\t') ||
            !std::getline(row, failedAttemptsStr, '\t') ||
            !std::getline(row, isLockedStr, '\t') ||
            !stringToRole(roleStr, user.role) ||
            !parseInteger(failedAttemptsStr, user.failedAttempts)) {
            users_.clear();
            return false;
        }
        user.isLocked = (isLockedStr == "1");
        std::getline(row, user.studentRollNumber);  // Optional roll number
        users_.push_back(std::move(user));
    }
    return true;
}

bool UserRepository::save() const {
    std::ofstream output(filePath_, std::ios::trunc);
    if (!output) {
        return false;
    }
    for (const UserAccount& user : users_) {
        output << user.username << '\t' << user.password << '\t'
               << roleToString(user.role) << '\t' << user.failedAttempts << '\t'
               << (user.isLocked ? "1" : "0") << '\t'
               << user.studentRollNumber << '\n';
    }
    return output.good();
}

bool UserRepository::addUser(UserAccount user) {
    if (findUser(user.username) != nullptr) {
        return false;
    }
    users_.push_back(std::move(user));
    return save();
}

UserAccount* UserRepository::findUser(const std::string& username) {
    for (UserAccount& user : users_) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

const UserAccount* UserRepository::findUser(const std::string& username) const {
    for (const UserAccount& user : users_) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

const std::vector<UserAccount>& UserRepository::users() const {
    return users_;
}

AuthService::AuthService(std::filesystem::path userFile)
    : repository_(std::move(userFile)),
      storageReady_(repository_.load()) {}

OperationResult AuthService::registerUser(const std::string& username,
                                          const std::string& password,
                                          Role role,
                                          const std::string& studentRollNumber) {
    if (!validField(username) || !validField(password)) {
        return {false, ErrorCode::InvalidInput, "Username and password must be non-empty."};
    }
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready."};
    }
    UserAccount user{username, password, role, 0, false, studentRollNumber};
    if (!repository_.addUser(std::move(user))) {
        return {false, ErrorCode::InvalidInput, "Username already exists or save failed."};
    }
    return {true, ErrorCode::None, "User registered successfully."};
}

LoginResult AuthService::login(const std::string& username,
                               const std::string& password) {
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready.", {}};
    }

    UserAccount* user = repository_.findUser(username);
    if (user == nullptr) {
        return {false, ErrorCode::InvalidCredentials, "Invalid username or password.", {}};
    }

    // SRM-F-002: If account is locked, 4th attempt is refused even if password is correct
    if (user->isLocked) {
        return {false, ErrorCode::AccountLocked,
                "Account is locked due to repeated failed login attempts. Contact an Administrator to unlock.", {}};
    }

    if (user->password != password) {
        user->failedAttempts++;
        if (user->failedAttempts >= 3) {
            user->isLocked = true;
            repository_.save();
            return {false, ErrorCode::AccountLocked,
                    "Account locked after 3 consecutive failed login attempts.", {}};
        }
        repository_.save();
        return {false, ErrorCode::InvalidCredentials, "Invalid username or password.", {}};
    }

    // Password correct: reset failed attempts and log in
    user->failedAttempts = 0;
    repository_.save();

    Session session{user->role, true, user->studentRollNumber};
    return {true, ErrorCode::None, "Login successful.", session};
}

OperationResult AuthService::unlockAccount(const Session& adminSession,
                                           const std::string& username) {
    // SRM-SR-003: Only an Administrator can unlock the account
    const auto auth = AccessController::authorize(adminSession, Operation::UnlockAccount);
    if (!auth.success) {
        return auth;
    }
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready."};
    }

    UserAccount* user = repository_.findUser(username);
    if (user == nullptr) {
        return {false, ErrorCode::StudentNotFound, "User account not found."};
    }

    user->isLocked = false;
    user->failedAttempts = 0;
    if (!repository_.save()) {
        return {false, ErrorCode::StorageFailure, "Failed to save unlocked user state."};
    }

    return {true, ErrorCode::None, "Account unlocked successfully."};
}

bool AuthService::isAccountLocked(const std::string& username) const {
    const auto* user = repository_.findUser(username);
    return user != nullptr && user->isLocked;
}

int AuthService::getFailedAttempts(const std::string& username) const {
    const auto* user = repository_.findUser(username);
    return user ? user->failedAttempts : 0;
}

}  // namespace srm

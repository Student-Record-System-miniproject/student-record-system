#include "srm/auth_service.h"
#include "srm/audit_service.h"
#include "srm/storage_utils.h"
#include "srm/input_validator.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {

namespace {

LoginResult rejectUnknownUser(
    const std::string& username,
    AuditService* auditService) {

    if (auditService) {
        auditService->logLogin(
            username, Role::Student, false, "Invalid username");
    }

    return {false, ErrorCode::InvalidCredentials,
            "Invalid username or password.", {}};
}

LoginResult rejectInactiveAccount(
    const UserAccount& user,
    AuditService* auditService) {

    if (auditService) {
        auditService->logLogin(
            user.username, user.role, false,
            "Login rejected: Account is deactivated");
    }

    return {false, ErrorCode::AccessDenied,
            "Account is deactivated. Contact an Administrator.", {}};
}

LoginResult rejectLockedAccount(
    const UserAccount& user,
    AuditService* auditService) {

    if (auditService) {
        auditService->logLogin(
            user.username, user.role, false,
            "Login rejected: Account is locked");
    }

    return {false, ErrorCode::AccountLocked,
            "Account is locked due to repeated failed login attempts. Contact an Administrator to unlock.",
            {}};
}

LoginResult handleAccountLockout(
    AuditService* auditService,
    const UserAccount& user,
    const std::string& username) {

    if (auditService) {
        auditService->logLogin(
            username, user.role, false,
            "Account locked after 3 consecutive failed login attempts");

        auditService->logOperation(
            Session{user.role, false, user.studentRollNumber, username,
                    std::chrono::steady_clock::time_point{}},
            "ACCOUNT_LOCKED", username, true,
            "Account locked due to 3 failed attempts");
    }

    return {false, ErrorCode::AccountLocked,
            "Account locked after 3 consecutive failed login attempts.", {}};
}

LoginResult handleFailedPassword(
    const UserRepository& repository,
    AuditService* auditService,
    UserAccount& user,
    const std::string& username) {

    const int previousFailed = user.failedAttempts;
    const bool previouslyLocked = user.isLocked;

    user.failedAttempts++;

    if (user.failedAttempts >= 3) {
        user.isLocked = true;
    }

    if (!repository.save()) {
        user.failedAttempts = previousFailed;
        user.isLocked = previouslyLocked;

        return {false, ErrorCode::StorageFailure,
                "Failed to persist login attempt state.", {}};
    }

    if (user.isLocked) {
        return handleAccountLockout(auditService, user, username);
    }

    if (auditService) {
        auditService->logLogin(
            username, user.role, false,
            "Failed password attempt " +
                std::to_string(user.failedAttempts));
    }

    return {false, ErrorCode::InvalidCredentials,
            "Invalid username or password.", {}};
}

bool resetFailedAttempts(
    const UserRepository& repository,
    UserAccount& user) {

    if (user.failedAttempts <= 0) {
        return true;
    }

    const int previousFailed = user.failedAttempts;
    user.failedAttempts = 0;

    if (!repository.save()) {
        user.failedAttempts = previousFailed;
        return false;
    }

    return true;
}

LoginResult createSuccessfulLogin(
    const UserAccount& user,
    AuditService* auditService) {

    if (auditService) {
        auditService->logLogin(
            user.username, user.role, true,
            "Successful authentication");
    }

    Session session{
        user.role,
        true,
        user.studentRollNumber,
        user.username,
        std::chrono::steady_clock::now()
    };

    return {true, ErrorCode::None, "Login successful.", session};
}

}  // namespace

AuthService::AuthService(std::filesystem::path userFile, unsigned int hashIterations)
    : repository_(std::move(userFile)),
      hashIterations_(hashIterations),
      storageReady_(repository_.load()) {}

bool AuthService::hasUsers() const {
    return !repository_.users().empty();
}

OperationResult AuthService::setupInitialAdmin(const std::string& username,
                                              const std::string& password) {
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready."};
    }
    if (!InputValidator::validateText(username) || !InputValidator::validateText(password)) {
        return {false, ErrorCode::InvalidInput, "Username and password must be non-empty and valid."};
    }
    if (!repository_.users().empty()) {
        return {false, ErrorCode::AccessDenied,
                "Initial administrator setup is disabled because accounts already exist."};
    }
    std::string hashedPassword = Crypto::hashPassword(password, "", hashIterations_);
    UserAccount adminUser{username, std::move(hashedPassword), Role::Administrator, 0, false, ""};
    if (!repository_.addUser(std::move(adminUser))) {
        return {false, ErrorCode::StorageFailure, "Failed to persist initial administrator account."};
    }
    return {true, ErrorCode::None, "Initial administrator created successfully."};
}

OperationResult AuthService::registerUser(const Session& adminSession,
                                          const std::string& username,
                                          const std::string& password,
                                          Role role,
                                          const std::string& studentRollNumber) {
    const auto auth = AccessController::authorize(adminSession, Operation::RegisterUser);
    if (!auth.success) {
        return auth;
    }
    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready."};
    }
    if (!InputValidator::validateText(username) ||
        !InputValidator::validateText(password) ||
        (!studentRollNumber.empty() &&
         !InputValidator::validateText(studentRollNumber))) {
        return {false, ErrorCode::InvalidInput,
                "Username, password, or student roll number contains invalid input."};

}
    if (repository_.findUser(username) != nullptr) {
        return {false, ErrorCode::DuplicateRecord, "User with this username already exists."};
    }
    std::string hashedPassword = Crypto::hashPassword(password, "", hashIterations_);
    UserAccount user{username, std::move(hashedPassword), role, 0, false, studentRollNumber};
    if (!repository_.addUser(std::move(user))) {
        return {false, ErrorCode::StorageFailure, "Failed to persist user to storage."};
    }
    if (auditService_) {
        auditService_->logOperation(adminSession, "REGISTER_USER", username, true,
                                    "Registered new " + roleToString(role) + " account");
    }
    return {true, ErrorCode::None, "User registered successfully."};
}

LoginResult AuthService::login(
    const std::string& username,
    const std::string& password) {

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure,
                "User storage is not ready.", {}};
    }

    if (!InputValidator::validateText(username) ||
        !InputValidator::validateText(password)) {
        return {false, ErrorCode::InvalidInput,
                "Username or password contains invalid input.", {}};
    }

    UserAccount* user = repository_.findUser(username);

    if (user == nullptr) {
        return rejectUnknownUser(username, auditService_);
    }

    if (!user->isActive) {
        return rejectInactiveAccount(*user, auditService_);
    }

    if (user->isLocked) {
        return rejectLockedAccount(*user, auditService_);
    }

    if (!Crypto::verifyPassword(password, user->passwordHash)) {
        return handleFailedPassword(
            repository_, auditService_, *user, username);
    }

    if (!resetFailedAttempts(repository_, *user)) {
        return {false, ErrorCode::StorageFailure,
                "Failed to persist login session state.", {}};
    }

    return createSuccessfulLogin(*user, auditService_);
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

    // Validate username before searching for the account.
    if (!InputValidator::validateText(username)) {
        return {false, ErrorCode::InvalidInput,
                "Username contains invalid input."};
    }

    UserAccount* user = repository_.findUser(username);
    if (user == nullptr) {
        return {false, ErrorCode::StudentNotFound, "User account not found."};
    }

    bool prevLocked = user->isLocked;
    int prevFailed = user->failedAttempts;

    user->isLocked = false;
    user->failedAttempts = 0;
    if (!repository_.save()) {
        user->isLocked = prevLocked;
        user->failedAttempts = prevFailed;
        return {false, ErrorCode::StorageFailure, "Failed to save unlocked user state."};
    }

    if (auditService_) {
        auditService_->logOperation(adminSession, "ACCOUNT_UNLOCKED", username, true,
                                    "Administrator unlocked user account");
    }

    return {true, ErrorCode::None, "Account unlocked successfully."};
}

OperationResult AuthService::deactivateAccount(const Session& adminSession,
                                                const std::string& username) {
    // Only an Administrator can deactivate a user account.
    const auto auth = AccessController::authorize(adminSession, Operation::ManageUsers);
    if (!auth.success) {
        return auth;
    }

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready."};
    }

    // Validate username before searching for the account.
    if (!InputValidator::validateText(username)) {
        return {false, ErrorCode::InvalidInput,
                "Username contains invalid input."};
    }

    UserAccount* user = repository_.findUser(username);
    if (user == nullptr) {
        return {false, ErrorCode::StudentNotFound, "User account not found."};
    }

    bool previousActive = user->isActive;

    user->isActive = false;

    if (!repository_.save()) {
        user->isActive = previousActive;
        return {false, ErrorCode::StorageFailure,
                "Failed to save deactivated user state."};
    }

    if (auditService_) {
        auditService_->logOperation(adminSession, "ACCOUNT_DEACTIVATED", username, true,
                                    "Administrator deactivated user account");
    }

    return {true, ErrorCode::None, "Account deactivated successfully."};
}

OperationResult AuthService::resetPassword(const Session& adminSession,
                                            const std::string& username,
                                            const std::string& newPassword) {
    // Only an Administrator can reset a user's password.
    const auto auth = AccessController::authorize(adminSession, Operation::ManageUsers);
    if (!auth.success) {
        return auth;
    }

    if (!storageReady_) {
        return {false, ErrorCode::StorageFailure, "User storage is not ready."};
    }

    // Validate username and new password.
    if (!InputValidator::validateText(username) ||
        !InputValidator::validateText(newPassword)) {
        return {false, ErrorCode::InvalidInput,
                "Username or password contains invalid input."};
    }

    UserAccount* user = repository_.findUser(username);
    if (user == nullptr) {
        return {false, ErrorCode::StudentNotFound, "User account not found."};
    }

    // Save the old password hash in case persistence fails.
    std::string previousPasswordHash = user->passwordHash;

    user->passwordHash = Crypto::hashPassword(newPassword, "", hashIterations_);

    if (!repository_.save()) {
        user->passwordHash = previousPasswordHash;
        return {false, ErrorCode::StorageFailure,
                "Failed to save the new password."};
    }

    if (auditService_) {
        auditService_->logOperation(adminSession, "PASSWORD_RESET", username, true,
                                    "Administrator reset user password");
    }

    return {true, ErrorCode::None, "Password reset successfully."};
}

bool AuthService::isAccountLocked(const std::string& username) const {
    const auto* user = repository_.findUser(username);
    return user != nullptr && user->isLocked;
}

int AuthService::getFailedAttempts(const std::string& username) const {
    const auto* user = repository_.findUser(username);
    return user ? user->failedAttempts : 0;
}

void AuthService::setAuditService(AuditService* service, bool requireAudit) {
    auditService_ = service;
    requireAudit_ = requireAudit;
}

AuditService* AuthService::auditService() const {
    return auditService_;
}

bool AuthService::isAuditRequired() const {
    return requireAudit_;
}

}  // namespace srm

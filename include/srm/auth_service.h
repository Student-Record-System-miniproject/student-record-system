
#pragma once

#include "srm/common_types.h"
#include "srm/crypto.h"
#include "srm/rbac.h"
#include "srm/user_repository.h"

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

class AuditService;

struct LoginResult {
    bool success = false;
    ErrorCode error = ErrorCode::InvalidCredentials;
    std::string message;
    Session session;
};

class AuthService {
public:
    explicit AuthService(std::filesystem::path userFile,
                         unsigned int hashIterations = 600000);

    OperationResult setupInitialAdmin(const std::string& username,
                                      const std::string& password);
    bool hasUsers() const;

    OperationResult registerUser(const Session& adminSession,
                                 const std::string& username,
                                 const std::string& password,
                                 Role role,
                                 const std::string& studentRollNumber = "");

    LoginResult login(const std::string& username,
                      const std::string& password);

    OperationResult unlockAccount(const Session& adminSession,
                                  const std::string& username);
    OperationResult deactivateAccount(const Session& adminSession,
                                  const std::string& username);

    OperationResult resetPassword(const Session& adminSession,
                                  const std::string& username,
                                  const std::string& newPassword);

    // SRM-F-004: Authenticated users can change their own passwords.
    OperationResult changePassword(const Session& session,
                                   const std::string& newPassword);

    bool isAccountLocked(const std::string& username) const;
    int getFailedAttempts(const std::string& username) const;

    void setAuditService(AuditService* auditService, bool requireAudit = false);
    AuditService* auditService() const;
    bool isAuditRequired() const;

private:
    bool validateNewPassword(const std::string& password) const;

    OperationResult persistPasswordChange(
        UserAccount& user,
        const std::string& newPassword);

    UserRepository repository_;
    unsigned int hashIterations_;
    bool storageReady_;
    AuditService* auditService_ = nullptr;
    bool requireAudit_ = false;
};

}  // namespace srm

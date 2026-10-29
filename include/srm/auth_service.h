#pragma once

#include "srm/common_types.h"
#include "srm/crypto.h"
#include "srm/rbac.h"

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

struct UserAccount {
    std::string username;
    std::string passwordHash;
    Role role;
    int failedAttempts = 0;
    bool isLocked = false;
    std::string studentRollNumber = "";
};

struct LoginResult {
    bool success;
    ErrorCode error;
    std::string message;
    Session session;
};

class UserRepository {
public:
    explicit UserRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;
    bool addUser(UserAccount user);
    UserAccount* findUser(const std::string& username);
    const UserAccount* findUser(const std::string& username) const;
    const std::vector<UserAccount>& users() const;

private:
    std::filesystem::path filePath_;
    std::vector<UserAccount> users_;
};

class AuthService {
public:
    explicit AuthService(std::filesystem::path userFile,
                         unsigned int hashIterations = 10000);

    // Initial setup mechanism: creates the first Administrator account.
    // Allowed only when no user accounts exist in the repository.
    OperationResult setupInitialAdmin(const std::string& username,
                                      const std::string& password);

    // SRM-SR-002: User registration requires an Administrator session (Operation::RegisterUser)
    OperationResult registerUser(const Session& adminSession,
                                 const std::string& username,
                                 const std::string& password,
                                 Role role,
                                 const std::string& studentRollNumber = "");

    // SRM-F-002: User login with account lockout
    // - Locks account after 3 consecutive failed login attempts
    // - 4th login attempt is refused even if password is correct
    // - Logs lockout event via AccessController
    // - Persists locked state to storage across restarts
    LoginResult login(const std::string& username,
                      const std::string& password);

    // SRM-SR-003: Admin-only account unlock
    // - Only Administrator may unlock a locked account
    // - Resets failed attempts to 0 and unlocks account
    // - Persists unlocked state
    OperationResult unlockAccount(const Session& adminSession,
                                  const std::string& username);

    bool isAccountLocked(const std::string& username) const;
    int getFailedAttempts(const std::string& username) const;

private:
    UserRepository repository_;
    unsigned int hashIterations_;
    bool storageReady_;
};

}  // namespace srm

#pragma once

#include "srm/common_types.h"

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

struct UserAccount {
    std::string username;
    std::string passwordHash;
    Role role = Role::Student;
    int failedAttempts = 0;
    bool isLocked = false;
    std::string studentRollNumber = "";
    bool isActive = true;
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

}  // namespace srm
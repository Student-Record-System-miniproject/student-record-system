#ifndef SRM_AUTH_MANAGER_H
#define SRM_AUTH_MANAGER_H

#include "file_repository.h"
#include "models.h"

#include <string>
#include <vector>

namespace srm {

class AuthManager {
public:
    explicit AuthManager(FileRepository& repository);

    bool registerUser(
        const std::string& username,
        const std::string& password,
        const std::string& role);

    bool login(
        const std::string& username,
        const std::string& password,
        Session& session);

    bool userExists(const std::string& username) const;

private:
    FileRepository& repository;
    std::vector<User> users;

    bool loadUsers();
    bool saveUsers();
};

} // namespace srm

#endif
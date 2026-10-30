#include "srm/auth_manager.h"

namespace srm {

AuthManager::AuthManager(FileRepository& repository)
    : repository(repository) {
    loadUsers();
}

bool AuthManager::loadUsers() {
    return repository.loadUsers(users);
}

bool AuthManager::saveUsers() {
    return repository.saveUsers(users);
}

bool AuthManager::userExists(
    const std::string& username) const {

    for (const auto& user : users) {
        if (user.username == username) {
            return true;
        }
    }

    return false;
}

bool AuthManager::registerUser(
    const std::string& username,
    const std::string& password,
    const std::string& role) {

    if (username.empty() || password.empty() || role.empty()) {
        return false;
    }

    if (userExists(username)) {
        return false;
    }

    User user;
    user.username = username;
    user.password = password;
    user.role = role;

    users.push_back(user);

    if (!saveUsers()) {
        users.pop_back();
        return false;
    }

    return true;
}

bool AuthManager::login(
    const std::string& username,
    const std::string& password,
    Session& session) {

    for (const auto& user : users) {
        if (user.username == username &&
            user.password == password) {

            session.userId = user.username;
            session.role = user.role;

            return true;
        }
    }

    return false;
}

} // namespace srm
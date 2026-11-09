#include "srm/user_repository.h"

#include "srm/input_validator.h"
#include "srm/storage_utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {

UserRepository::UserRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool UserRepository::load() {
    recoverInterruptedSave(filePath_);
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
            !std::getline(row, user.passwordHash, '\t') ||
            !std::getline(row, roleStr, '\t') ||
            !std::getline(row, failedAttemptsStr, '\t') ||
            !std::getline(row, isLockedStr, '\t') ||
            !stringToRole(roleStr, user.role) ||
            !InputValidator::parseInteger(
                failedAttemptsStr, user.failedAttempts)) {
            users_.clear();
            return false;
        }

        user.isLocked = (isLockedStr == "1");
        std::string activeText;

        if (std::getline(row, user.studentRollNumber, '\t') &&
            std::getline(row, activeText)) {
            if (activeText != "0" && activeText != "1") {
                users_.clear();
                return false;
            }

            user.isActive = (activeText == "1");
        } else {
            // Existing account records without this field remain active.
            user.isActive = true;
        }

        users_.push_back(std::move(user));
    }

    return true;
}

bool UserRepository::save() const {
    return atomicWriteFile(filePath_, [&](std::ostream& output) {
        for (const UserAccount& user : users_) {
            output << user.username << '\t' << user.passwordHash << '\t'
                   << roleToString(user.role) << '\t' << user.failedAttempts
                   << '\t' << (user.isLocked ? "1" : "0") << '\t'
                   << user.studentRollNumber << '\t'
                   << (user.isActive ? "1" : "0") << '\n';
        }
        return output.good();
    });
}

bool UserRepository::addUser(UserAccount user) {
    if (findUser(user.username) != nullptr) {
        return false;
    }

    users_.push_back(std::move(user));

    if (!save()) {
        users_.pop_back();
        return false;
    }

    return true;
}

UserAccount* UserRepository::findUser(const std::string& username) {
    for (UserAccount& user : users_) {
        if (user.username == username) {
            return &user;
        }
    }

    return nullptr;
}

const UserAccount* UserRepository::findUser(
    const std::string& username) const {
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

}  // namespace srm
#ifndef SRM_MODELS_H
#define SRM_MODELS_H

#include <string>

namespace srm {

struct Student {
    std::string rollNumber;
    std::string name;
    std::string department;
    std::string semester;
};

struct User {
    std::string username;
    std::string password;
    std::string role;
};

struct Session {
    std::string userId;
    std::string role;
};

} // namespace srm

#endif
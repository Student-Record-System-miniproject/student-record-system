#include "srm/auth_service.h"
#include "srm/common_types.h"

#include <iostream>
#include <string>

void displayMenu(const srm::Session& session) {
    std::cout << "\nLogin successful.\n";
    std::cout << "Logged in as: " << session.username << "\n";
    std::cout << "Role: " << srm::roleToString(session.role) << "\n";

    if (session.role == srm::Role::Administrator) {
        std::cout << "\nAdministrator Menu\n";
        std::cout << "1. Manage users\n";
        std::cout << "2. Manage students\n";
        std::cout << "3. View reports\n";
    }
    else if (session.role == srm::Role::Teacher) {
        std::cout << "\nTeacher Menu\n";
        std::cout << "1. Manage student records\n";
        std::cout << "2. Enter marks\n";
        std::cout << "3. View reports\n";
    }
    else if (session.role == srm::Role::Student) {
        std::cout << "\nStudent Menu\n";
        std::cout << "1. View profile\n";
        std::cout << "2. View marks\n";
        std::cout << "3. View report card\n";
    }
}

int main() {
    srm::AuthService authService("data/users.txt");

    std::string username;
    std::string password;

    std::cout << "Student Record System\n";
    std::cout << "Username: ";
    std::cin >> username;

    std::cout << "Password: ";
    std::cin >> password;

    srm::LoginResult result = authService.login(username, password);

    if (!result.success) {
        std::cout << "\n" << result.message << "\n";
        std::cout << "Login failed.\n";
        return 1;
    }

    displayMenu(result.session);

    return 0;
}
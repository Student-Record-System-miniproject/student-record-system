#include "srm/auth_manager.h"
#include "srm/file_repository.h"

#include <iostream>
#include <string>
#include <vector>

void displayMenu(const srm::Session& session) {
    std::cout << "\nLogin successful.\n";
    std::cout << "Logged in as: " << session.userId << "\n";
    std::cout << "Role: " << session.role << "\n";

    if (session.role == "ADMIN") {
        std::cout << "\nAdministrator Menu\n";
        std::cout << "1. Manage users\n";
        std::cout << "2. Manage students\n";
        std::cout << "3. View reports\n";
    }
    else if (session.role == "TEACHER") {
        std::cout << "\nTeacher Menu\n";
        std::cout << "1. Manage student records\n";
        std::cout << "2. Enter marks\n";
        std::cout << "3. View reports\n";
    }
    else if (session.role == "STUDENT") {
        std::cout << "\nStudent Menu\n";
        std::cout << "1. View profile\n";
        std::cout << "2. View marks\n";
        std::cout << "3. View report card\n";
    }
}

int main() {
    srm::FileRepository repository("data");
    srm::AuthManager authManager(repository);

    std::string username;
    std::string password;

    std::cout << "Student Record System\n";
    std::cout << "Username: ";
    std::cin >> username;

    std::cout << "Password: ";
    std::cin >> password;

    srm::Session session;

    if (!authManager.login(username, password, session)) {
        std::cout << "\nInvalid username or password.\n";
        std::cout << "Login failed.\n";
        return 1;
    }

    displayMenu(session);

    return 0;
}
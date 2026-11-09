#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/common_types.h"
#include "srm/input_validator.h"
#include "srm/rbac.h"
#include "srm/student_service.h"
#include "srm/directory_security.h"
#include <chrono>
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
    // SRM-SR-006: Protect data and backup directories before accessing files.
    if (!srm::DirectorySecurity::protectApplicationDirectories(
            "data", "backups")) {
        std::cerr << "SECURITY ERROR: Unable to protect application data "
                     "and backup directories.\n";
        return 1;
    }
    // Initialize central audit logging service (SRM-F-044, SRM-NF-008, SRM-SR-005)
    srm::AuditService auditService("data/audit.log");

    // Route RBAC access violations and security events to the audit service
    srm::AccessController::setEventLogger([&auditService](const srm::SecurityEvent& ev) {
        auditService.logSecurityViolation(ev, ev.username.empty() ? "anonymous" : ev.username);
    });

    // Wire audit logging to authentication service
    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);

    // Wire audit logging to student service (addressing review feedback)
    srm::StudentService studentService("data/students.txt");
    studentService.setAuditService(&auditService);

    while (true) {
        std::string username;
        std::string password;

        std::cout << "\nStudent Record System\n";
        std::cout << "Username: ";

        if (!std::getline(std::cin, username)) {
            return 0;
        }

        std::cout << "Password: ";

        if (!std::getline(std::cin, password)) {
            return 0;
        }

        srm::LoginResult result = authService.login(username, password);

        if (!result.success) {
            std::cout << "\n" << result.message << "\n";
            std::cout << "Login failed.\n";
            return 1;
        }

        srm::Session session = result.session;
        displayMenu(session);

        while (session.authenticated) {
            std::cout << "\nEnter your choice: ";

            std::string choice;
            if (!std::getline(std::cin, choice)) {
                return 0;
            }

            int choiceNumber = 0;
            if (!srm::InputValidator::parseInteger(choice, choiceNumber) ||
                !srm::InputValidator::validateRange(choiceNumber, 1, 3)) {
                std::cout << "Invalid choice. Please enter a number between 1 and 3.\n";
                continue;
            }

            if (srm::isSessionExpired(session)) {
                session.authenticated = false;
                auditService.logOperation(session, "SESSION_EXPIRED", session.username, true,
                                          "Session timed out due to 5 minutes inactivity");
                std::cout << "\nSession expired due to 5 minutes of inactivity.\n";
                std::cout << "Please log in again.\n";
                break;
            }

            std::cout << "Action " << choiceNumber << " selected.\n";
            auditService.logOperation(session, "MENU_ACTION", "Option_" + std::to_string(choiceNumber), true);

            // Execute student management action when selected
            if (choiceNumber == 2 && session.role == srm::Role::Administrator) {
                const auto students = studentService.searchStudents(session, "");
                std::cout << "Current registered student count: " << students.size() << "\n";
            }

            session.lastActivity = std::chrono::steady_clock::now();
        }
    }

    return 0;
}

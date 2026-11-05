#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/common_types.h"
#include "srm/rbac.h"

#include <iostream>
#include <string>
#include <chrono>

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
    // Initialize central audit logging service (SRM-F-044, SRM-NF-008, SRM-SR-005)
    srm::AuditService auditService("data/audit.log");

    // Route RBAC access violations and security events to the audit service
    srm::AccessController::setEventLogger([&auditService](const srm::SecurityEvent& ev) {
        auditService.logSecurityViolation(ev, ev.username.empty() ? "anonymous" : ev.username);
    });

    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);

    while (true) {
        std::string username;
        std::string password;

        std::cout << "\nStudent Record System\n";
        std::cout << "Username: ";
        if (!(std::cin >> username)) {
            break;
        }

        std::cout << "Password: ";
        if (!(std::cin >> password)) {
            break;
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
            if (!(std::cin >> choice)) {
                break;
            }

            if (srm::isSessionExpired(session)) {
                session.authenticated = false;
                auditService.logOperation(session, "SESSION_EXPIRED", session.username, true,
                                          "Session timed out due to 5 minutes inactivity");
                std::cout << "\nSession expired due to 5 minutes of inactivity.\n";
                std::cout << "Please log in again.\n";
                break;
            }

            std::cout << "Action " << choice << " selected.\n";
            auditService.logOperation(session, "MENU_ACTION", "Option_" + choice, true);
            session.lastActivity = std::chrono::steady_clock::now();
        }
    }

    return 0;
}

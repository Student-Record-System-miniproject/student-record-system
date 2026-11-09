
#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/backup_service.h"
#include "srm/common_types.h"
#include "srm/input_validator.h"
#include "srm/rbac.h"
#include "srm/student_service.h"

#include <chrono>
#include <cstddef>
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
        std::cout << "4. Create Backup\n";
        std::cout << "5. Restore Backup\n";
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

    // Initialize central audit logging service
    srm::AuditService auditService("data/audit.log");

    // Initialize backup and restore service (SRM-F-041, SRM-F-042)
    srm::BackupService backupService(
        "data",
        "backups",
        &auditService
    );

    // Route RBAC security events to the audit service
    srm::AccessController::setEventLogger(
        [&auditService](const srm::SecurityEvent& ev) {
            auditService.logSecurityViolation(
                ev,
                ev.username.empty() ? "anonymous" : ev.username
            );
        }
    );

    // Initialize authentication service
    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);

    // Initialize student service
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

            // Administrators have five menu options.
            // Teachers and students have three.
            const int maxChoice =
                session.role == srm::Role::Administrator ? 5 : 3;

            int choiceNumber = 0;

            if (!srm::InputValidator::parseInteger(
                    choice, choiceNumber) ||
                !srm::InputValidator::validateRange(
                    choiceNumber, 1, maxChoice)) {

                std::cout << "Invalid menu choice. Please try again.\n";
                continue;
            }

            // Check session expiration
            if (srm::isSessionExpired(session)) {

                session.authenticated = false;

                auditService.logOperation(
                    session,
                    "SESSION_EXPIRED",
                    session.username,
                    true,
                    "Session timed out due to 5 minutes inactivity"
                );

                std::cout << "\nSession expired due to 5 minutes of inactivity.\n";
                std::cout << "Please log in again.\n";

                break;
            }

            std::cout << "Action " << choiceNumber << " selected.\n";

            auditService.logOperation(
                session,
                "MENU_ACTION",
                "Option_" + std::to_string(choiceNumber),
                true
            );

            // Option 2: Manage students
            if (choiceNumber == 2 &&
                session.role == srm::Role::Administrator) {

                const auto students =
                    studentService.searchStudents(session, "");

                std::cout << "Current registered student count: "
                          << students.size() << "\n";
            }

            // SRM-F-041: Create Backup
            if (choiceNumber == 4 &&
                session.role == srm::Role::Administrator) {

                std::cout << "\nCreating system backup...\n";

                const auto backupResult =
                    backupService.createBackup(session);

                std::cout << backupResult.message << "\n";
            }

            // SRM-F-042: Restore Backup
            if (choiceNumber == 5 &&
                session.role == srm::Role::Administrator) {

                const auto backups = backupService.listBackups();

                if (backups.empty()) {

                    std::cout << "\nNo backups available.\n";

                } else {

                    std::cout << "\nAvailable Backups:\n";

                    for (std::size_t i = 0; i < backups.size(); ++i) {
                        std::cout << i + 1 << ". "
                                  << backups[i].filename().string()
                                  << "\n";
                    }

                    std::cout << "\nSelect backup number: ";

                    std::string backupChoice;

                    if (!std::getline(std::cin, backupChoice)) {
                        return 0;
                    }

                    int selectedBackup = 0;

                    if (!srm::InputValidator::parseInteger(
                            backupChoice, selectedBackup) ||
                        selectedBackup < 1 ||
                        selectedBackup >
                            static_cast<int>(backups.size())) {

                        std::cout << "Invalid backup selection.\n";

                    } else {

                        std::cout
                            << "\nWARNING: Restoring will replace current data.\n";

                        std::cout
                            << "Type Y to confirm restoration: ";

                        std::string confirmation;

                        if (!std::getline(std::cin, confirmation)) {
                            return 0;
                        }

                        const auto restoreResult =
                            backupService.restoreBackup(
                                session,
                                backups[selectedBackup - 1]
                                    .filename().string(),
                                confirmation
                            );

                        std::cout << restoreResult.message << "\n";
                    }
                }
            }

            // Update session activity timestamp
            session.lastActivity =
                std::chrono::steady_clock::now();
        }
    }

    return 0;
}


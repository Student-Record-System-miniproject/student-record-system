
#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/common_types.h"
#include "srm/input_validator.h"
#include "srm/rbac.h"
#include "srm/student_service.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

void displayMenu(const srm::Session& session) {
    std::cout << "\n================================\n";
    std::cout << "       STUDENT RECORD SYSTEM\n";
    std::cout << "================================\n";
    std::cout << "Logged in as: " << session.username << "\n";
    std::cout << "Role: " << srm::roleToString(session.role) << "\n";

    if (session.role == srm::Role::Administrator) {
        std::cout << "\nAdministrator Menu\n";
        std::cout << "1. View student records\n";
        std::cout << "2. View student count\n";
        std::cout << "3. Logout\n";
    }
    else if (session.role == srm::Role::Teacher) {
        std::cout << "\nTeacher Menu\n";
        std::cout << "1. View student records\n";
        std::cout << "2. Logout\n";
    }
    else if (session.role == srm::Role::Student) {
        std::cout << "\nStudent Menu\n";
        std::cout << "1. View my profile\n";
        std::cout << "2. Logout\n";
    }

    std::cout << "0. Exit application\n";
}

void displayStudents(const std::vector<srm::StudentRecord>& students) {
    if (students.empty()) {
        std::cout << "\nNo student records found.\n";
        std::cout << "Next step: Contact an Administrator if you expected records.\n";
        return;
    }

    std::cout << "\nRegistered Student Records\n";
    std::cout << "---------------------------------------------\n";

    for (const auto& student : students) {
        std::cout << "Roll Number : " << student.rollNumber << "\n";
        std::cout << "Name        : " << student.name << "\n";
        std::cout << "Department  : " << student.department << "\n";
        std::cout << "Email       : " << student.email << "\n";
        std::cout << "---------------------------------------------\n";
    }

    std::cout << "Total records: " << students.size() << "\n";
}

int main() {
    // Initialize audit logging.
    srm::AuditService auditService("data/audit.log");

    srm::AccessController::setEventLogger(
        [&auditService](const srm::SecurityEvent& ev) {
            auditService.logSecurityViolation(
                ev,
                ev.username.empty() ? "anonymous" : ev.username
            );
        }
    );

    // Initialize authentication and student services.
    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);

    srm::StudentService studentService("data/students.txt");
    studentService.setAuditService(&auditService);

    // Create the first Administrator account when user storage is empty.
    if (std::filesystem::exists("data/users.txt") &&
        std::filesystem::file_size("data/users.txt") == 0) {

        std::cout << "\n================================\n";
        std::cout << "   INITIAL ADMINISTRATOR SETUP\n";
        std::cout << "================================\n";
        std::cout << "No user accounts were found.\n";
        std::cout << "Create the first Administrator account.\n";

        std::string adminUsername;
        std::string adminPassword;

        std::cout << "\nCreate Administrator username: ";
        if (!std::getline(std::cin, adminUsername)) {
            return 0;
        }

        std::cout << "Create Administrator password: ";
        if (!std::getline(std::cin, adminPassword)) {
            return 0;
        }

        const auto setupResult =
            authService.setupInitialAdmin(adminUsername, adminPassword);

        if (!setupResult.success) {
            std::cout << "\nAdministrator setup failed: "
                      << setupResult.message << "\n";
            std::cout << "Next step: Check your inputs and user storage permissions.\n";
            return 1;
        }

        std::cout << "\nAdministrator account created successfully!\n";
        std::cout << "You can now log in using your new credentials.\n";
    }

    // Main login loop.
    while (true) {
        std::string username;
        std::string password;

        std::cout << "\n================================\n";
        std::cout << "      STUDENT RECORD SYSTEM\n";
        std::cout << "================================\n";

        std::cout << "Enter username (or type EXIT to quit): ";

        if (!std::getline(std::cin, username)) {
            return 0;
        }

        if (username == "EXIT" || username == "exit") {
            std::cout << "Exiting application. Goodbye!\n";
            return 0;
        }

        std::cout << "Enter password: ";

        if (!std::getline(std::cin, password)) {
            return 0;
        }

        srm::LoginResult result = authService.login(username, password);

        if (!result.success) {
            std::cout << "\nLogin failed: " << result.message << "\n";

            if (result.error == srm::ErrorCode::AccountLocked) {
                std::cout << "Next step: Contact an Administrator to unlock your account.\n";
                return 1;
            }

            if (result.error == srm::ErrorCode::StorageFailure) {
                std::cout << "Next step: Check the user data file and storage permissions.\n";
                return 1;
            }

            std::cout << "Next step: Verify your username and password, then try again.\n";
            continue;
        }

        srm::Session session = result.session;

        std::cout << "\nLogin successful!\n";

        bool loggedIn = true;

        // Role-based menu loop.
        while (loggedIn && session.authenticated) {
            displayMenu(session);

            std::cout << "\nEnter your choice: ";

            std::string choice;

            if (!std::getline(std::cin, choice)) {
                return 0;
            }

            int choiceNumber = 0;

            if (!srm::InputValidator::parseInteger(choice, choiceNumber)) {
                std::cout << "\nInvalid input.\n";
                std::cout << "Next step: Enter a whole number from the displayed menu.\n";
                continue;
            }

            if (choiceNumber == 0) {
                std::cout << "Exiting Student Record System. Goodbye!\n";
                return 0;
            }

            int maxChoice =
                session.role == srm::Role::Administrator ? 3 : 2;

            if (!srm::InputValidator::validateRange(
                    choiceNumber, 1, maxChoice)) {

                std::cout << "\nInvalid menu choice.\n";
                std::cout << "Next step: Select an option between 1 and "
                          << maxChoice << " from the menu.\n";
                continue;
            }

            // Check session timeout.
            if (srm::isSessionExpired(session)) {
                session.authenticated = false;

                auditService.logOperation(
                    session,
                    "SESSION_EXPIRED",
                    session.username,
                    true,
                    "Session timed out due to 5 minutes inactivity"
                );

                std::cout << "\nYour session expired after 5 minutes of inactivity.\n";
                std::cout << "Next step: Log in again to continue.\n";
                break;
            }

            // Administrator actions.
            if (session.role == srm::Role::Administrator) {
                if (choiceNumber == 1) {
                    displayStudents(studentService.listStudents(session));
                }
                else if (choiceNumber == 2) {
                    const auto students = studentService.listStudents(session);

                    std::cout << "\nRegistered student count: "
                              << students.size() << "\n";
                }
                else if (choiceNumber == 3) {
                    std::cout << "\nLogging out successfully.\n";
                    loggedIn = false;
                }
            }

            // Teacher actions.
            else if (session.role == srm::Role::Teacher) {
                if (choiceNumber == 1) {
                    displayStudents(studentService.listStudents(session));
                }
                else if (choiceNumber == 2) {
                    std::cout << "\nLogging out successfully.\n";
                    loggedIn = false;
                }
            }

            // Student actions.
            else if (session.role == srm::Role::Student) {
                if (choiceNumber == 1) {
                    const auto* student = studentService.getStudent(
                        session,
                        session.studentRollNumber
                    );

                    if (student == nullptr) {
                        std::cout << "\nYour student profile could not be found.\n";
                        std::cout << "Next step: Contact an Administrator to verify your account details.\n";
                    }
                    else {
                        std::cout << "\nMy Student Profile\n";
                        std::cout << "---------------------------------------------\n";
                        std::cout << "Roll Number : " << student->rollNumber << "\n";
                        std::cout << "Name        : " << student->name << "\n";
                        std::cout << "Department  : " << student->department << "\n";
                        std::cout << "Email       : " << student->email << "\n";
                        std::cout << "---------------------------------------------\n";
                    }
                }
                else if (choiceNumber == 2) {
                    std::cout << "\nLogging out successfully.\n";
                    loggedIn = false;
                }
            }

            auditService.logOperation(
                session,
                "MENU_ACTION",
                "Option_" + std::to_string(choiceNumber),
                true
            );

            session.lastActivity = std::chrono::steady_clock::now();
        }
    }

    return 0;
}

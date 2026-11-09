#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/common_types.h"
#include "srm/input_validator.h"
#include "srm/rbac.h"
#include "srm/student_service.h"

#include <chrono>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

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

bool readMaskedPassword(const std::string& prompt,
                        std::string& password) {
    std::cout << prompt;
    password.clear();

#ifdef _WIN32
    while (true) {
        int ch = _getch();

        if (ch == '\r') {
            std::cout << '\n';
            return true;
        }

        if (ch == '\b') {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b" << std::flush;
            }
            continue;
        }

        // Ignore special keys such as arrow keys.
        if (ch == 0 || ch == 224) {
            (void)_getch();
            continue;
        }

        if (ch == 3) {
            std::cout << '\n';
            return false;
        }

        if (ch >= 32 && ch != 127) {
            password.push_back(static_cast<char>(ch));
            std::cout << '*' << std::flush;
        }
    }

#else
    termios originalSettings{};

    if (tcgetattr(STDIN_FILENO, &originalSettings) != 0) {
        std::cerr << "\nUnable to configure masked password input.\n";
        return false;
    }

    termios maskedSettings = originalSettings;
    maskedSettings.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON));
    maskedSettings.c_cc[VMIN] = 1;
    maskedSettings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &maskedSettings) != 0) {
        std::cerr << "\nUnable to configure masked password input.\n";
        return false;
    }

    while (true) {
        char ch;

        if (!std::cin.get(ch)) {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
            std::cout << '\n';
            return false;
        }

        if (ch == '\n' || ch == '\r') {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
            std::cout << '\n';
            return true;
        }

        if (ch == '\b' || ch == 127) {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b" << std::flush;
            }
            continue;
        }

        if (static_cast<unsigned char>(ch) >= 32) {
            password.push_back(ch);
            std::cout << '*' << std::flush;
        }
    }
#endif
}

void manageUsersMenu(srm::AuthService& authService,
                     const srm::Session& session) {
    while (true) {
        std::cout << "\nUser Management\n";
        std::cout << "1. Create user account\n";
        std::cout << "2. Deactivate user account\n";
        std::cout << "3. Unlock user account\n";
        std::cout << "4. Reset user password\n";
        std::cout << "5. Back to main menu\n";
        std::cout << "Enter your choice: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return;
        }

        int choiceNumber = 0;
        if (!srm::InputValidator::parseInteger(choice, choiceNumber) ||
            choiceNumber < 1 || choiceNumber > 5) {
            std::cout << "Invalid choice. Please enter a number between 1 and 5.\n";
            continue;
        }

        if (choiceNumber == 5) {
            return;
        }

        std::string username;
        std::cout << "Enter username: ";
        if (!std::getline(std::cin, username)) {
            return;
        }

        srm::OperationResult result;

        if (choiceNumber == 1) {
            std::string password;
            std::string roleInput;
            std::string rollNumber;
            int roleChoice = 0;

            if (!readMaskedPassword("Enter initial password: ", password)) {
                return;
            }

            std::cout << "Select role:\n";
            std::cout << "1. Administrator\n";
            std::cout << "2. Teacher\n";
            std::cout << "3. Student\n";
            std::cout << "Enter role choice: ";

            if (!std::getline(std::cin, roleInput)) {
                return;
            }

            if (!srm::InputValidator::parseInteger(roleInput, roleChoice) ||
                roleChoice < 1 || roleChoice > 3) {
                std::cout << "Invalid role selection.\n";
                continue;
            }

            srm::Role role = srm::Role::Administrator;

            if (roleChoice == 2) {
                role = srm::Role::Teacher;
            } else if (roleChoice == 3) {
                role = srm::Role::Student;

                std::cout << "Enter student roll number: ";
                if (!std::getline(std::cin, rollNumber)) {
                    return;
                }
            }

            result = authService.registerUser(
                session, username, password, role, rollNumber);
        }
        else if (choiceNumber == 2) {
            result = authService.deactivateAccount(session, username);
        }
        else if (choiceNumber == 3) {
            result = authService.unlockAccount(session, username);
        }
        else if (choiceNumber == 4) {
            std::string newPassword;

            if (!readMaskedPassword("Enter new password: ", newPassword)) {
    return;
}

            result = authService.resetPassword(
                session, username, newPassword);
        }

        std::cout << result.message << "\n";
    }
}

int main() {
    // Initialize central audit logging service (SRM-F-044, SRM-NF-008, SRM-SR-005)
    srm::AuditService auditService("data/audit.log");

    // Route RBAC access violations and security events to the audit service
    srm::AccessController::setEventLogger([&auditService](const srm::SecurityEvent& ev) {
        auditService.logSecurityViolation(ev, ev.username.empty() ? "anonymous" : ev.username);
    });

    // Wire audit logging to authentication service
    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);

    // Create the first Administrator account if no users exist.
    if (!authService.hasUsers()) {
        std::cout << "\nNo user accounts found.\n";
        std::cout << "Create the initial Administrator account.\n";

        std::string adminUsername;
        std::string adminPassword;

        std::cout << "Choose Administrator username: ";
        if (!std::getline(std::cin, adminUsername)) {
            return 0;
        }

        if (!readMaskedPassword("Choose Administrator password: ", adminPassword)) {
            return 0;
        }

        const auto setupResult =
            authService.setupInitialAdmin(adminUsername, adminPassword);

        std::cout << setupResult.message << "\n";

        if (!setupResult.success) {
            return 1;
        }
    }

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

        if (!readMaskedPassword("Password: ", password)) {
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

            // Execute user management action when selected
            if (choiceNumber == 1 && session.role == srm::Role::Administrator) {
                manageUsersMenu(authService, session);
            }

            // Execute student management action when selected
            else if (choiceNumber == 2 && session.role == srm::Role::Administrator) {
                const auto students = studentService.searchStudents(session, "");
                std::cout << "Current registered student count: " << students.size() << "\n";
            }

            session.lastActivity = std::chrono::steady_clock::now();
        }
    }

    return 0;
}

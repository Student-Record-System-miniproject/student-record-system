#include "srm/attendance_service.h"
#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/backup_service.h"
#include "srm/common_types.h"
#include "srm/directory_security.h"
#include "srm/grade_service.h"
#include "srm/input_validator.h"
#include "srm/rbac.h"
#include "srm/student_service.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace {

enum class LoginStatus { Authenticated, Exit, FatalError };
enum class UserMenuActionStatus { Completed, Retry, Exit };

bool readLine(const std::string& prompt, std::string& value) {
    std::cout << prompt;
    return static_cast<bool>(std::getline(std::cin, value));
}

bool readInteger(const std::string& prompt, int& value) {
    std::string input;
    if (!readLine(prompt, input)) {
        return false;
    }
    if (!srm::InputValidator::parseInteger(input, value)) {
        std::cout << "Invalid input. Next step: Enter a whole number.\n";
        return false;
    }
    return true;
}

void printResult(const srm::OperationResult& result) {
    std::cout << (result.success ? "Success: " : "Error: ")
              << result.message << '\n';
}

#ifdef _WIN32
bool readMaskedPasswordPlatform(std::string& password) {
    while (true) {
        const int ch = _getch();
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
}
#else
bool readMaskedPasswordPlatform(std::string& password) {
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
        char ch = '\0';
        if (!std::cin.get(ch)) {
            (void)tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
            std::cout << '\n';
            return false;
        }
        if (ch == '\n' || ch == '\r') {
            (void)tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
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
}
#endif

bool readMaskedPassword(const std::string& prompt, std::string& password) {
    std::cout << prompt;
    password.clear();
    return readMaskedPasswordPlatform(password);
}

void displayMenu(const srm::Session& session) {
    std::cout << "\n================================\n"
              << "       STUDENT RECORD SYSTEM\n"
              << "================================\n"
              << "Logged in as: " << session.username << '\n'
              << "Role: " << srm::roleToString(session.role) << '\n';

    if (session.role == srm::Role::Administrator) {
        std::cout << "\nAdministrator Menu\n"
                  << "1. User Management\n"
                  << "2. Student Management\n"
                  << "3. View Audit Logs / Reports\n"
                  << "4. Create Backup\n"
                  << "5. Restore Backup\n"
                  << "6. Logout\n";
    } else if (session.role == srm::Role::Teacher) {
        std::cout << "\nTeacher Menu\n"
                  << "1. View Student Records\n"
                  << "2. Enter Marks\n"
                  << "3. View Student Reports\n"
                  << "4. Mark Attendance\n"
                  << "5. Logout\n";
    } else if (session.role == srm::Role::Student) {
        std::cout << "\nStudent Menu\n"
                  << "1. View My Profile\n"
                  << "2. View Marks / Report Card\n"
                  << "3. View Attendance Report\n"
                  << "4. Logout\n";
    }
    std::cout << "0. Exit application\n";
}

void displayStudents(const std::vector<srm::StudentRecord>& students) {
    if (students.empty()) {
        std::cout << "\nNo student records found.\n"
                  << "Next step: Contact an Administrator if you expected records.\n";
        return;
    }

    std::cout << "\nStudent Records\n"
              << "---------------------------------------------\n";
    for (const auto& student : students) {
        std::cout << "Roll Number : " << student.rollNumber << '\n'
                  << "Name        : " << student.name << '\n'
                  << "Department  : " << student.department << '\n'
                  << "Email       : " << student.email << '\n'
                  << "Year        : " << student.year << '\n'
                  << "---------------------------------------------\n";
    }
    std::cout << "Total records: " << students.size() << '\n';
}

UserMenuActionStatus createUserAccount(
    srm::AuthService& authService,
    const srm::Session& session,
    const std::string& username,
    srm::OperationResult& result) {
    std::string password;
    std::string roleInput;
    std::string rollNumber;
    int roleChoice = 0;

    if (!readMaskedPassword("Enter initial password: ", password)) {
        return UserMenuActionStatus::Exit;
    }
    if (!readLine("Select role (1 Administrator, 2 Teacher, 3 Student): ",
                  roleInput)) {
        return UserMenuActionStatus::Exit;
    }
    if (!srm::InputValidator::parseInteger(roleInput, roleChoice) ||
        roleChoice < 1 || roleChoice > 3) {
        std::cout << "Invalid role selection. Next step: Enter 1, 2, or 3.\n";
        return UserMenuActionStatus::Retry;
    }

    srm::Role role = srm::Role::Administrator;
    if (roleChoice == 2) {
        role = srm::Role::Teacher;
    } else if (roleChoice == 3) {
        role = srm::Role::Student;
        if (!readLine("Enter student roll number: ", rollNumber)) {
            return UserMenuActionStatus::Exit;
        }
    }

    result = authService.registerUser(session, username, password, role, rollNumber);
    return UserMenuActionStatus::Completed;
}

UserMenuActionStatus resetUserPassword(
    srm::AuthService& authService,
    const srm::Session& session,
    const std::string& username,
    srm::OperationResult& result) {
    std::string newPassword;
    if (!readMaskedPassword("Enter new password: ", newPassword)) {
        return UserMenuActionStatus::Exit;
    }
    result = authService.resetPassword(session, username, newPassword);
    return UserMenuActionStatus::Completed;
}

UserMenuActionStatus processUserManagementChoice(
    srm::AuthService& authService,
    const srm::Session& session,
    int choiceNumber) {
    std::string username;
    if (!readLine("Enter username: ", username)) {
        return UserMenuActionStatus::Exit;
    }

    srm::OperationResult result;
    if (choiceNumber == 1) {
        const auto status = createUserAccount(authService, session, username, result);
        if (status != UserMenuActionStatus::Completed) {
            return status;
        }
    } else if (choiceNumber == 2) {
        result = authService.deactivateAccount(session, username);
    } else if (choiceNumber == 3) {
        result = authService.unlockAccount(session, username);
    } else if (choiceNumber == 4) {
        const auto status = resetUserPassword(authService, session, username, result);
        if (status != UserMenuActionStatus::Completed) {
            return status;
        }
    }

    printResult(result);
    return UserMenuActionStatus::Completed;
}

void manageUsersMenu(srm::AuthService& authService,
                     const srm::Session& session) {
    while (true) {
        std::cout << "\nUser Management\n"
                  << "1. Create user account\n"
                  << "2. Deactivate user account\n"
                  << "3. Unlock user account\n"
                  << "4. Reset user password\n"
                  << "5. Back to main menu\n";
        int choice = 0;
        if (!readInteger("Enter your choice: ", choice)) {
            if (!std::cin) return;
            continue;
        }
        if (choice == 5) return;
        if (choice < 1 || choice > 4) {
            std::cout << "Invalid choice. Next step: Enter a number from 1 to 5.\n";
            continue;
        }

        const auto status = processUserManagementChoice(authService, session, choice);
        if (status == UserMenuActionStatus::Exit) return;
    }
}

void manageStudentsMenu(srm::StudentService& service,
                        const srm::Session& session) {
    while (true) {
        std::cout << "\nStudent Management\n"
                  << "1. Add student\n"
                  << "2. Update student\n"
                  << "3. Delete student\n"
                  << "4. View all student records\n"
                  << "5. Back to main menu\n";
        int choice = 0;
        if (!readInteger("Enter your choice: ", choice)) {
            if (!std::cin) return;
            continue;
        }
        if (choice == 5) return;
        if (choice < 1 || choice > 4) {
            std::cout << "Invalid choice. Next step: Enter a number from 1 to 5.\n";
            continue;
        }

        if (choice == 1) {
            srm::StudentRecord student;
            if (!readLine("Roll number: ", student.rollNumber) ||
                !readLine("Name: ", student.name) ||
                !readLine("Department: ", student.department) ||
                !readLine("Email: ", student.email) ||
                !readLine("Date of birth (YYYY-MM-DD): ", student.dateOfBirth) ||
                !readLine("Contact number: ", student.contactNumber)) {
                return;
            }
            if (!readInteger("Year (1-4): ", student.year)) {
                if (!std::cin) return;
                continue;
            }
            printResult(service.addStudent(session, student));
        } else if (choice == 2) {
            std::string roll, name, department, email;
            if (!readLine("Student roll number to update: ", roll) ||
                !readLine("New name: ", name) ||
                !readLine("New department: ", department) ||
                !readLine("New email: ", email)) {
                return;
            }
            printResult(service.updateStudent(session, roll, name, department, email));
        } else if (choice == 3) {
            std::string roll, confirmation;
            if (!readLine("Student roll number to delete: ", roll) ||
                !readLine("Confirm deletion (Y/N): ", confirmation)) {
                return;
            }
            printResult(service.deleteStudent(session, roll, confirmation));
        } else if (choice == 4) {
            displayStudents(service.listStudents(session));
        }
    }
}

void viewAuditLogs(srm::AuditService& auditService,
                   const srm::Session& session) {
    const auto authorization =
        srm::AccessController::authorize(session, srm::Operation::ViewAuditLogs);
    if (!authorization.success) {
        printResult(authorization);
        return;
    }

    const auto entries = auditService.query();
    if (entries.empty()) {
        std::cout << "\nNo audit log entries found.\n"
                  << "Next step: Perform an application operation and check again.\n";
        return;
    }

    std::cout << "\nAudit Log / Reports\n"
              << "---------------------------------------------\n";
    for (const auto& entry : entries) {
        std::cout << entry.timestamp << " | " << entry.username << " | "
                  << srm::roleToString(entry.role) << " | " << entry.action
                  << " | " << entry.targetResource << " | " << entry.status
                  << " | " << entry.details << '\n';
    }
    std::cout << "Total entries: " << entries.size() << '\n';
}

void viewStudentReports(srm::GradeService& grades,
                        srm::AttendanceService& attendance,
                        const srm::Session& session,
                        const std::string& rollNumber) {
    const auto gradeReport = grades.generateReportCard(session, rollNumber);
    if (gradeReport.success) {
        std::cout << '\n' << gradeReport.report;
    } else {
        std::cout << "Report card unavailable: " << gradeReport.message << '\n';
    }

    const auto attendanceReport = attendance.generateReport(session, rollNumber);
    if (attendanceReport.success) {
        std::cout << '\n' << attendanceReport.report;
    } else {
        std::cout << "Attendance report unavailable: "
                  << attendanceReport.message << '\n';
    }
}

bool markStudentAttendance(srm::AttendanceService& service,
                           const srm::Session& session) {
    std::string rollNumber, subject, date, statusInput;
    std::cout << "\nMark Student Attendance\n";
    if (!readLine("Student roll number: ", rollNumber) ||
        !readLine("Subject: ", subject) ||
        !readLine("Date (YYYY-MM-DD): ", date) ||
        !readLine("Attendance (P for Present / A for Absent): ", statusInput)) {
        return false;
    }

    srm::AttendanceStatus status;
    if (statusInput == "P" || statusInput == "p") {
        status = srm::AttendanceStatus::Present;
    } else if (statusInput == "A" || statusInput == "a") {
        status = srm::AttendanceStatus::Absent;
    } else {
        std::cout << "Invalid attendance status. Next step: Enter P or A.\n";
        return true;
    }

    printResult(service.markAttendance(session, rollNumber, subject, date, status));
    return true;
}

bool createInitialAdminIfNeeded(srm::AuthService& authService,
                                int& exitCode) {
    if (authService.hasUsers()) {
        return true;
    }

    std::cout << "\n================================\n"
              << "   INITIAL ADMINISTRATOR SETUP\n"
              << "================================\n"
              << "No user accounts were found.\n"
              << "Create the first Administrator account.\n";

    std::string adminUsername;
    std::string adminPassword;
    if (!readLine("Choose Administrator username: ", adminUsername)) {
        exitCode = 0;
        return false;
    }
    if (!readMaskedPassword("Choose Administrator password: ", adminPassword)) {
        exitCode = 0;
        return false;
    }

    const auto result = authService.setupInitialAdmin(adminUsername, adminPassword);
    printResult(result);
    if (!result.success) {
        std::cout << "Next step: Check the supplied details and user storage permissions.\n";
        exitCode = 1;
        return false;
    }
    std::cout << "You can now log in using your new credentials.\n";
    return true;
}

LoginStatus promptLogin(srm::AuthService& authService, srm::Session& session) {
    while (true) {
        std::string username;
        std::string password;
        std::cout << "\n================================\n"
                  << "      STUDENT RECORD SYSTEM\n"
                  << "================================\n";

        if (!readLine("Enter username (or type EXIT to quit): ", username)) {
            return LoginStatus::Exit;
        }
        if (username == "EXIT" || username == "exit") {
            std::cout << "Exiting application. Goodbye!\n";
            return LoginStatus::Exit;
        }
        if (!readMaskedPassword("Enter password: ", password)) {
            return LoginStatus::Exit;
        }

        const auto result = authService.login(username, password);
        if (result.success) {
            session = result.session;
            std::cout << "\nLogin successful!\n";
            return LoginStatus::Authenticated;
        }

        std::cout << "\nLogin failed: " << result.message << '\n';
        if (result.error == srm::ErrorCode::StorageFailure) {
            std::cout << "Next step: Check the user data file and storage permissions.\n";
            return LoginStatus::FatalError;
        }
        if (result.error == srm::ErrorCode::AccountLocked) {
            std::cout << "Next step: Contact an Administrator to unlock this account. "
                         "You may try logging in with another account.\n";
        } else {
            std::cout << "Next step: Verify your username and password, then try again.\n";
        }
    }
}

bool restoreBackupFromMenu(srm::BackupService& backupService,
                           const srm::Session& session) {
    const auto backups = backupService.listBackups();
    if (backups.empty()) {
        std::cout << "\nNo backups available.\n"
                  << "Next step: Create a backup before attempting restoration.\n";
        return true;
    }

    std::cout << "\nAvailable Backups:\n";
    for (std::size_t i = 0; i < backups.size(); ++i) {
        std::cout << i + 1 << ". " << backups[i].filename().string() << '\n';
    }

    std::string backupChoice;
    if (!readLine("Select backup number: ", backupChoice)) {
        return false;
    }
    int selectedBackup = 0;
    if (!srm::InputValidator::parseInteger(backupChoice, selectedBackup) ||
        selectedBackup < 1 ||
        selectedBackup > static_cast<int>(backups.size())) {
        std::cout << "Invalid backup selection. Next step: Select one of the listed numbers.\n";
        return true;
    }

    std::cout << "\nWARNING: Restoring will replace current data.\n";
    std::string confirmation;
    if (!readLine("Type Y to confirm restoration: ", confirmation)) {
        return false;
    }

    const auto result = backupService.restoreBackup(
        session,
        backups[static_cast<std::size_t>(selectedBackup - 1)].filename().string(),
        confirmation);
    printResult(result);
    return true;
}

bool runAuthenticatedSession(
    srm::Session& session,
    srm::AuthService& authService,
    srm::StudentService& studentService,
    srm::GradeService& gradeService,
    srm::AttendanceService& attendanceService,
    srm::BackupService& backupService,
    srm::AuditService& auditService) {
    while (session.authenticated) {
        displayMenu(session);
        int choice = 0;
        if (!readInteger("Enter your choice: ", choice)) {
            if (!std::cin) return false;
            continue;
        }

        if (choice == 0) {
            std::cout << "Exiting application. Goodbye!\n";
            session.authenticated = false;
            return false;
        }

        const int maxChoice =
            session.role == srm::Role::Administrator ? 6 :
            session.role == srm::Role::Teacher ? 5 : 4;
        if (choice < 1 || choice > maxChoice) {
            std::cout << "Invalid menu choice. Next step: Enter a number from 0 to "
                      << maxChoice << ".\n";
            continue;
        }

        if (srm::isSessionExpired(session)) {
            session.authenticated = false;
            auditService.logOperation(
                session, "SESSION_EXPIRED", session.username, true,
                "Session timed out due to 5 minutes inactivity");
            std::cout << "\nYour session expired after 5 minutes of inactivity.\n"
                      << "Next step: Log in again to continue.\n";
            return true;
        }

        auditService.logOperation(
            session, "MENU_ACTION", "Option_" + std::to_string(choice), true);

        bool logout = false;
        if (session.role == srm::Role::Administrator) {
            if (choice == 1) {
                manageUsersMenu(authService, session);
            } else if (choice == 2) {
                manageStudentsMenu(studentService, session);
            } else if (choice == 3) {
                viewAuditLogs(auditService, session);
            } else if (choice == 4) {
                std::cout << "\nCreating system backup...\n";
                printResult(backupService.createBackup(session));
            } else if (choice == 5) {
                if (!restoreBackupFromMenu(backupService, session)) return false;
            } else if (choice == 6) {
                logout = true;
            }
        } else if (session.role == srm::Role::Teacher) {
            if (choice == 1) {
                displayStudents(studentService.listStudents(session));
            } else if (choice == 2) {
                std::cout << "Marks entry is unavailable because no subject catalogue "
                             "is configured. Subject validation has not been bypassed.\n"
                          << "Next step: Configure an approved subject catalogue before entering marks.\n";
            } else if (choice == 3) {
                std::string rollNumber;
                if (!readLine("Student roll number for reports: ", rollNumber)) return false;
                viewStudentReports(gradeService, attendanceService, session, rollNumber);
            } else if (choice == 4) {
                if (!markStudentAttendance(attendanceService, session)) return false;
            } else if (choice == 5) {
                logout = true;
            }
        } else if (session.role == srm::Role::Student) {
            if (choice == 1) {
                const auto* student =
                    studentService.getStudent(session, session.studentRollNumber);
                if (student == nullptr) {
                    std::cout << "Your student profile could not be found.\n"
                              << "Next step: Contact an Administrator to verify your account details.\n";
                } else {
                    std::cout << "\nMy Profile\n"
                              << "Roll Number : " << student->rollNumber << '\n'
                              << "Name        : " << student->name << '\n'
                              << "Department  : " << student->department << '\n'
                              << "Email       : " << student->email << '\n'
                              << "Date of Birth: " << student->dateOfBirth << '\n'
                              << "Year        : " << student->year << '\n'
                              << "Contact     : " << student->contactNumber << '\n';
                }
            } else if (choice == 2) {
                const auto report = gradeService.generateReportCard(
                    session, session.studentRollNumber);
                if (report.success) {
                    std::cout << '\n' << report.report;
                } else {
                    std::cout << "Report card unavailable: " << report.message << '\n';
                }
            } else if (choice == 3) {
                const auto report = attendanceService.generateReport(
                    session, session.studentRollNumber);
                if (report.success) {
                    std::cout << '\n' << report.report;
                } else {
                    std::cout << "Attendance report unavailable: "
                              << report.message << '\n';
                }
            } else if (choice == 4) {
                logout = true;
            }
        }

        if (logout) {
            std::cout << "\nLogging out successfully.\n";
            session.authenticated = false;
            return true;
        }
        session.lastActivity = std::chrono::steady_clock::now();
    }
    return true;
}

}  // namespace

int main() {
    // Protect sensitive directories before any application data file is opened.
    if (!srm::DirectorySecurity::protectApplicationDirectories("data", "backups")) {
        std::cerr << "SECURITY ERROR: Unable to protect application data "
                     "and backup directories.\n";
        return 1;
    }

    srm::AuditService auditService("data/audit.log");
    srm::BackupService backupService("data", "backups", &auditService);

    srm::AccessController::setEventLogger(
        [&auditService](const srm::SecurityEvent& event) {
            auditService.logSecurityViolation(
                event, event.username.empty() ? "anonymous" : event.username);
        });

    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);

    int setupExitCode = 0;
    if (!createInitialAdminIfNeeded(authService, setupExitCode)) {
        return setupExitCode;
    }

    srm::StudentService studentService("data/students.txt");
    studentService.setAuditService(&auditService);

    srm::GradeService gradeService(
        "data/grades.txt",
        [&studentService](const std::string& rollNumber) {
            return studentService.exists(rollNumber);
        },
        [](const std::string&) {
            // No subject catalogue exists; do not permit unvalidated marks entry.
            return false;
        });

    srm::AttendanceService attendanceService(
        "data/attendance.txt",
        [&studentService](const std::string& rollNumber) {
            return studentService.exists(rollNumber);
        });

    while (true) {
        srm::Session session{};
        const LoginStatus status = promptLogin(authService, session);
        if (status == LoginStatus::Exit) return 0;
        if (status == LoginStatus::FatalError) return 1;

        if (!runAuthenticatedSession(session, authService, studentService,
                                     gradeService, attendanceService,
                                     backupService, auditService)) {
            return 0;
        }
    }
}

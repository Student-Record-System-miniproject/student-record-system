#include "srm/attendance_service.h"
#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/common_types.h"
#include "srm/grade_service.h"
#include "srm/input_validator.h"
#include "srm/rbac.h"
#include "srm/student_service.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace {

enum class LoginStatus { Authenticated, Exit, FatalError };

bool readLine(const std::string& prompt, std::string& value) {
    std::cout << prompt;
    return static_cast<bool>(std::getline(std::cin, value));
}

bool readInteger(const std::string& prompt, int& value) {
    std::string input;
    if (!readLine(prompt, input)) return false;
    if (!srm::InputValidator::parseInteger(input, value)) {
        std::cout << "Invalid input. Please enter a valid number.\n";
        return false;
    }
    return true;
}

void printResult(const srm::OperationResult& result) {
    std::cout << (result.success ? "Success: " : "Error: ")
              << result.message << '\n';
}

void displayStudents(const std::vector<srm::StudentRecord>& students) {
    if (students.empty()) {
        std::cout << "\nNo student records found.\n";
        return;
    }
    std::cout << "\nStudent Records\n---------------------------------------------\n";
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

void displayMenu(const srm::Session& session) {
    std::cout << "\n================================\n"
              << "       STUDENT RECORD SYSTEM\n"
              << "================================\n"
              << "Logged in as: " << session.username << '\n'
              << "Role: " << srm::roleToString(session.role) << '\n';

    if (session.role == srm::Role::Administrator) {
        std::cout << "\nAdministrator Menu\n"
                  << "1. User Management\n2. Student Management\n"
                  << "3. View Audit Logs / Reports\n4. Logout\n";
    } else if (session.role == srm::Role::Teacher) {
        std::cout << "\nTeacher Menu\n"
                  << "1. View Student Records\n2. Enter Marks\n"
                  << "3. View Student Reports\n4. Mark Attendance\n5. Logout\n";
    } else if (session.role == srm::Role::Student) {
        std::cout << "\nStudent Menu\n"
                  << "1. View My Profile\n2. View Marks / Report Card\n"
                  << "3. View Attendance Report\n4. Logout\n";
    }
    std::cout << "0. Exit application\n";
}

void manageUsersMenu(srm::AuthService& authService,
                     const srm::Session& session) {
    while (true) {
        std::cout << "\nUser Management\n"
                  << "1. Create user account\n2. Deactivate user account\n"
                  << "3. Unlock user account\n4. Reset user password\n"
                  << "5. Back to main menu\n";
        int choice = 0;
        if (!readInteger("Enter your choice: ", choice)) {
            if (!std::cin) return;
            continue;
        }
        if (choice == 5) return;
        if (choice < 1 || choice > 4) {
            std::cout << "Invalid choice. Enter a number from 1 to 5.\n";
            continue;
        }

        std::string username;
        if (!readLine("Enter username: ", username)) return;
        srm::OperationResult result{false, srm::ErrorCode::InvalidInput,
                                    "No operation was performed."};

        if (choice == 1) {
            std::string password, roleInput, rollNumber;
            int roleChoice = 0;
            if (!readLine("Enter initial password: ", password) ||
                !readLine("Select role (1 Administrator, 2 Teacher, 3 Student): ",
                          roleInput)) return;
            if (!srm::InputValidator::parseInteger(roleInput, roleChoice) ||
                roleChoice < 1 || roleChoice > 3) {
                std::cout << "Invalid role selection.\n";
                continue;
            }
            srm::Role role = srm::Role::Administrator;
            if (roleChoice == 2) role = srm::Role::Teacher;
            if (roleChoice == 3) {
                role = srm::Role::Student;
                if (!readLine("Enter student roll number: ", rollNumber)) return;
            }
            result = authService.registerUser(session, username, password,
                                              role, rollNumber);
        } else if (choice == 2) {
            result = authService.deactivateAccount(session, username);
        } else if (choice == 3) {
            result = authService.unlockAccount(session, username);
        } else if (choice == 4) {
            std::string password;
            if (!readLine("Enter new password: ", password)) return;
            result = authService.resetPassword(session, username, password);
        }
        printResult(result);
    }
}

void manageStudentsMenu(srm::StudentService& service,
                        const srm::Session& session) {
    while (true) {
        std::cout << "\nStudent Management\n"
                  << "1. Add student\n2. Update student\n3. Delete student\n"
                  << "4. View all student records\n5. Back to main menu\n";
        int choice = 0;
        if (!readInteger("Enter your choice: ", choice)) {
            if (!std::cin) return;
            continue;
        }
        if (choice == 5) return;
        if (choice < 1 || choice > 4) {
            std::cout << "Invalid choice. Enter a number from 1 to 5.\n";
            continue;
        }

        if (choice == 1) {
            srm::StudentRecord student;
            if (!readLine("Roll number: ", student.rollNumber) ||
                !readLine("Name: ", student.name) ||
                !readLine("Department: ", student.department) ||
                !readLine("Email: ", student.email) ||
                !readLine("Date of birth (YYYY-MM-DD): ", student.dateOfBirth) ||
                !readLine("Contact number: ", student.contactNumber)) return;
            if (!readInteger("Year (1-4): ", student.year)) continue;
            printResult(service.addStudent(session, student));
        } else if (choice == 2) {
            std::string roll, name, department, email;
            if (!readLine("Student roll number to update: ", roll) ||
                !readLine("New name: ", name) ||
                !readLine("New department: ", department) ||
                !readLine("New email: ", email)) return;
            printResult(service.updateStudent(session, roll, name, department, email));
        } else if (choice == 3) {
            std::string roll, confirmation;
            if (!readLine("Student roll number to delete: ", roll) ||
                !readLine("Confirm deletion (Y/N): ", confirmation)) return;
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
        std::cout << "\nNo audit log entries found.\n";
        return;
    }
    std::cout << "\nAudit Log / Reports\n---------------------------------------------\n";
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
    if (gradeReport.success) std::cout << '\n' << gradeReport.report;
    else std::cout << "Report card unavailable: " << gradeReport.message << '\n';

    const auto attendanceReport = attendance.generateReport(session, rollNumber);
    if (attendanceReport.success) std::cout << '\n' << attendanceReport.report;
    else std::cout << "Attendance report unavailable: "
                   << attendanceReport.message << '\n';
}

void markStudentAttendance(srm::AttendanceService& service,
                           const srm::Session& session) {
    std::string roll, subject, date, statusInput;
    if (!readLine("Student roll number: ", roll) ||
        !readLine("Subject: ", subject) ||
        !readLine("Date (YYYY-MM-DD): ", date) ||
        !readLine("Attendance (P for Present / A for Absent): ", statusInput)) return;

    srm::AttendanceStatus status;
    if (statusInput == "P" || statusInput == "p")
        status = srm::AttendanceStatus::Present;
    else if (statusInput == "A" || statusInput == "a")
        status = srm::AttendanceStatus::Absent;
    else {
        std::cout << "Invalid attendance status. Enter P or A.\n";
        return;
    }
    printResult(service.markAttendance(session, roll, subject, date, status));
}

LoginStatus promptLogin(srm::AuthService& authService, srm::Session& session) {
    while (true) {
        std::string username, password;
        std::cout << "\n================================\n"
                  << "      STUDENT RECORD SYSTEM\n"
                  << "================================\n";
        if (!readLine("Enter username (or type EXIT to quit): ", username))
            return LoginStatus::Exit;
        if (username == "EXIT" || username == "exit") {
            std::cout << "Exiting application. Goodbye!\n";
            return LoginStatus::Exit;
        }
        if (!readLine("Enter password: ", password)) return LoginStatus::Exit;

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
                         "You may log in with another account.\n";
        } else {
            std::cout << "Next step: Verify your username and password, then try again.\n";
        }
    }
}

bool runAuthenticatedSession(
    srm::Session& session, srm::AuthService& authService,
    srm::StudentService& studentService, srm::GradeService& gradeService,
    srm::AttendanceService& attendanceService, srm::AuditService& auditService) {
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
            session.role == srm::Role::Administrator ? 4 :
            session.role == srm::Role::Teacher ? 5 : 4;
        if (choice < 1 || choice > maxChoice) {
            std::cout << "Invalid choice. Please enter a number between 0 and "
                      << maxChoice << ".\n";
            continue;
        }

        if (srm::isSessionExpired(session)) {
            session.authenticated = false;
            auditService.logOperation(session, "SESSION_EXPIRED",
                session.username, true,
                "Session timed out due to 5 minutes inactivity");
            std::cout << "\nYour session expired after 5 minutes of inactivity.\n"
                      << "Next step: Log in again to continue.\n";
            return true;
        }

        bool logout = false;
        if (session.role == srm::Role::Administrator) {
            if (choice == 1) manageUsersMenu(authService, session);
            else if (choice == 2) manageStudentsMenu(studentService, session);
            else if (choice == 3) viewAuditLogs(auditService, session);
            else if (choice == 4) logout = true;
        } else if (session.role == srm::Role::Teacher) {
            if (choice == 1) {
                displayStudents(studentService.listStudents(session));
            } else if (choice == 2) {
                std::cout << "Marks entry is unavailable because no subject "
                             "catalogue is configured. Subject validation has "
                             "not been bypassed.\n";
            } else if (choice == 3) {
                std::string roll;
                if (!readLine("Student roll number for reports: ", roll)) return false;
                viewStudentReports(gradeService, attendanceService, session, roll);
            } else if (choice == 4) {
                markStudentAttendance(attendanceService, session);
            } else if (choice == 5) {
                logout = true;
            }
        } else if (session.role == srm::Role::Student) {
            if (choice == 1) {
                const auto* student =
                    studentService.getStudent(session, session.studentRollNumber);
                if (student == nullptr) {
                    std::cout << "Your student profile could not be found.\n";
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
                if (report.success) std::cout << '\n' << report.report;
                else std::cout << "Report card unavailable: " << report.message << '\n';
            } else if (choice == 3) {
                const auto report = attendanceService.generateReport(
                    session, session.studentRollNumber);
                if (report.success) std::cout << '\n' << report.report;
                else std::cout << "Attendance report unavailable: "
                               << report.message << '\n';
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

bool createInitialAdminIfNeeded(srm::AuthService& authService) {
    if (authService.hasUsers()) return true;

    std::cout << "\nNo user accounts found.\n"
              << "Create the initial Administrator account.\n";
    std::string username, password;
    if (!readLine("Choose Administrator username: ", username) ||
        !readLine("Choose Administrator password: ", password)) return false;

    const auto result = authService.setupInitialAdmin(username, password);
    printResult(result);
    return result.success;
}

}  // namespace

int main() {
    srm::AuditService auditService("data/audit.log");
    srm::AccessController::setEventLogger(
        [&auditService](const srm::SecurityEvent& event) {
            auditService.logSecurityViolation(
                event, event.username.empty() ? "anonymous" : event.username);
        });

    srm::AuthService authService("data/users.txt");
    authService.setAuditService(&auditService);
    if (!createInitialAdminIfNeeded(authService)) return 1;

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
                                     auditService)) return 0;
    }
}

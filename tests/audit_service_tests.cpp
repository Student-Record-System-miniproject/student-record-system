#include "srm/audit_service.h"
#include "srm/auth_service.h"
#include "srm/rbac.h"
#include "srm/student_service.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

using srm::AccessController;
using srm::AuditEntry;
using srm::AuditService;
using srm::AuthService;
using srm::ErrorCode;
using srm::Operation;
using srm::Role;
using srm::SecurityEvent;
using srm::Session;
using srm::StudentRecord;
using srm::StudentService;

// TC-Adm-03: Verify audit and security event logging, retention, and append-only integrity
void testAuditLogLoginAndSecurityEvents() {
    const auto logPath = std::filesystem::temp_directory_path() / "srm_audit_test.log";
    std::filesystem::remove(logPath);

    AuditService audit(logPath);

    // 1. Log successful login
    assert(audit.logLogin("alice_teacher", Role::Teacher, true, "Successful login"));
    assert(audit.count() == 1);

    // 2. Log failed login
    assert(audit.logLogin("intruder", Role::Student, false, "Invalid password attempt 1"));
    assert(audit.count() == 2);

    // 3. Log security violation via AccessController integration
    AccessController::clearSecurityEvents();
    AccessController::setEventLogger([&](const SecurityEvent& ev) {
        audit.logSecurityViolation(ev, "bob_student");
    });

    const Session studentSession{Role::Student, true, "S001", "bob_student"};
    // Student attempting unauthorized marks entry triggers security event callback
    const auto authResult = AccessController::authorize(studentSession, Operation::EnterMarks);
    assert(!authResult.success);

    assert(audit.count() == 3);

    // Verify all entries contain timestamp and user information
    const auto entries = audit.query();
    assert(entries.size() == 3);

    for (const auto& entry : entries) {
        assert(!entry.timestamp.empty());
        assert(entry.timestamp.find('T') != std::string::npos);  // ISO 8601
        assert(entry.timestamp.back() == 'Z');
        assert(!entry.username.empty());
        assert(!entry.status.empty());
    }

    assert(entries[0].action == "LOGIN_SUCCESS");
    assert(entries[0].username == "alice_teacher");
    assert(entries[0].status == "SUCCESS");

    assert(entries[1].action == "LOGIN_FAILED");
    assert(entries[1].username == "intruder");
    assert(entries[1].status == "FAILURE");

    assert(entries[2].action == "SECURITY_VIOLATION");
    assert(entries[2].username == "bob_student");
    assert(entries[2].status == "DENIED");

    // Clean up hook
    AccessController::setEventLogger(nullptr);
    std::filesystem::remove(logPath);
}

void testAuditLogCRUDAndSystemOperations() {
    const auto logPath = std::filesystem::temp_directory_path() / "srm_audit_crud.log";
    std::filesystem::remove(logPath);

    AuditService audit(logPath);
    const Session adminSession{Role::Administrator, true, {}, "admin_root"};

    // Log Student CRUD operations
    assert(audit.logOperation(adminSession, "CREATE_STUDENT", "S101", true, "Added Alice"));
    assert(audit.logOperation(adminSession, "UPDATE_STUDENT", "S101", true, "Updated email"));
    assert(audit.logOperation(adminSession, "DELETE_STUDENT", "S101", true, "Confirmed delete"));

    // Log Backup and Restore operations
    assert(audit.logBackup(adminSession, "backup_2026.tar", true, "Full system backup"));
    assert(audit.logRestore(adminSession, "backup_2026.tar", true, "Restored state"));

    assert(audit.count() == 5);

    // Query filters
    const auto backupEntries = audit.query("", "BACKUP_DATA");
    assert(backupEntries.size() == 1);
    assert(backupEntries[0].username == "admin_root");
    assert(backupEntries[0].targetResource == "backup_2026.tar");

    const auto restoreEntries = audit.query("", "RESTORE_DATA");
    assert(restoreEntries.size() == 1);
    assert(restoreEntries[0].targetResource == "backup_2026.tar");

    std::filesystem::remove(logPath);
}

void testAppendOnlyIntegrityAndRetention() {
    const auto logPath = std::filesystem::temp_directory_path() / "srm_audit_retention.log";
    std::filesystem::remove(logPath);

    // Session 1: Write initial entries
    {
        AuditService audit(logPath);
        audit.logLogin("user1", Role::Teacher, true);
        audit.logLogin("user2", Role::Administrator, true);
        assert(audit.count() == 2);
    }

    // Inspect file directly
    std::string fileContentSession1;
    {
        std::ifstream in(logPath);
        fileContentSession1.assign((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
    }

    // Session 2: Reload and append more entries (simulating passage of time / application restarts)
    {
        AuditService reopenedAudit(logPath);
        assert(reopenedAudit.count() == 2);  // Past entries retained intact
        reopenedAudit.logLogin("user3", Role::Student, false, "Failed attempt");
        assert(reopenedAudit.count() == 3);
    }

    // Verify existing entries were never altered
    {
        std::ifstream in(logPath);
        std::string fileContentSession2((std::istreambuf_iterator<char>(in)),
                                        std::istreambuf_iterator<char>());
        // Previous content is an exact prefix of the new content (append-only guarantee)
        assert(fileContentSession2.rfind(fileContentSession1, 0) == 0);
        assert(fileContentSession2.find("user3") != std::string::npos);
    }

    std::filesystem::remove(logPath);
}

// Comment 4 verification: Log injection prevention (CWE-117)
void testLogInjectionPrevention() {
    const auto logPath = std::filesystem::temp_directory_path() / "srm_audit_injection.log";
    std::filesystem::remove(logPath);

    AuditService audit(logPath);

    // Attempt log injection using tab, carriage return, newline, and backslash characters
    const std::string maliciousUser = "attacker\tAdministrator\tLOGIN_SUCCESS\nfake_entry";
    const std::string maliciousDetails = "Line 1\r\nLine 2\tTabbed\nLine 3\\backslash";
    const std::string maliciousResource = "res\tname\nwith\nnewlines";

    assert(audit.logLogin(maliciousUser, Role::Student, false, maliciousDetails));

    // Verify exactly 1 entry was recorded despite newlines in fields
    assert(audit.count() == 1);

    // Verify raw file contains only 1 newline (at the end of the entry)
    {
        std::ifstream in(logPath);
        std::string rawContent((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
        std::size_t newlineCount = 0;
        for (char ch : rawContent) {
            if (ch == '\n') ++newlineCount;
        }
        assert(newlineCount == 1);
        // Literal tabs inside user and details should be escaped
        assert(rawContent.find("\\tAdministrator") != std::string::npos);
        assert(rawContent.find("\\nfake_entry") != std::string::npos);
    }

    // Verify query unescapes back accurately
    const auto entries = audit.query();
    assert(entries.size() == 1);
    assert(entries[0].username == maliciousUser);
    assert(entries[0].details == maliciousDetails);

    std::filesystem::remove(logPath);
}

// Comment 6 verification: Handling of malformed/corrupted rows
void testMalformedRowHandling() {
    const auto logPath = std::filesystem::temp_directory_path() / "srm_audit_corrupted.log";
    std::filesystem::remove(logPath);

    AuditService audit(logPath);
    assert(audit.logLogin("alice", Role::Teacher, true, "First entry"));

    // Manually inject malformed lines into the log file
    {
        std::ofstream out(logPath, std::ios::app);
        out << "only_one_field\n";
        out << "two\tfields\n";
        out << "2026-01-01T00:00:00Z\tuser\tNonExistentRole\tACTION\tRES\tSUCCESS\tdetails\n";
    }

    // Append another valid entry
    assert(audit.logLogin("bob", Role::Student, true, "Second valid entry"));

    // Query should parse the 2 valid entries and track the 3 malformed lines
    const auto validEntries = audit.query();
    assert(validEntries.size() == 2);
    assert(validEntries[0].username == "alice");
    assert(validEntries[1].username == "bob");
    assert(audit.malformedRowsCount() == 3);

    std::filesystem::remove(logPath);
}

// Review comment verification: Handling audit write failures and fail-secure vs fail-open behavior
void testAuditWriteFailureAndFailSecurePolicy() {
    // Point audit service to an invalid path that cannot be written
#if defined(_WIN32)
    const auto invalidLogPath = std::filesystem::path("Z:\\non_existent_drive\\invalid.log");
#else
    const auto invalidLogPath = std::filesystem::path("/non_existent_root_dir/invalid.log");
#endif

    AuditService failingAudit(invalidLogPath);

    // 1. Direct write returns false and tracks failure
    assert(!failingAudit.logLogin("alice", Role::Teacher, true));
    assert(failingAudit.lastWriteFailed());
    assert(failingAudit.writeFailureCount() >= 1);

    // 2. StudentService fail-secure policy test
    const auto studentPath = std::filesystem::temp_directory_path() / "srm_student_fail_secure.txt";
    std::filesystem::remove(studentPath);

    StudentService studentService(studentPath);
    const Session adminSession{Role::Administrator, true, {}, "admin"};
    const StudentRecord student{"S501", "Dave", "ECE", "dave@pes.edu"};

    // With fail-secure policy (requireAudit = true): operation fails if audit fails
    studentService.setAuditService(&failingAudit, true);
    assert(studentService.isAuditRequired());
    const auto secureRes = studentService.addStudent(adminSession, student);
    assert(!secureRes.success);
    assert(secureRes.error == ErrorCode::StorageFailure);
    assert(!studentService.exists("S501"));

    // With fail-open policy (requireAudit = false, default): operation succeeds with logged alert
    studentService.setAuditService(&failingAudit, false);
    assert(!studentService.isAuditRequired());
    const auto openRes = studentService.addStudent(adminSession, student);
    assert(openRes.success);
    assert(studentService.exists("S501"));

    std::filesystem::remove(studentPath);
}

// Comments 1 & 2 verification: End-to-end integration with AuthService and StudentService
void testServiceIntegrationWithAuditLogging() {
    const auto auditPath = std::filesystem::temp_directory_path() / "srm_audit_integration.log";
    const auto userPath = std::filesystem::temp_directory_path() / "srm_users_audit.txt";
    const auto studentPath = std::filesystem::temp_directory_path() / "srm_students_audit.txt";

    std::filesystem::remove(auditPath);
    std::filesystem::remove(userPath);
    std::filesystem::remove(studentPath);

    AuditService audit(auditPath);

    // 1. AuthService integration
    AuthService auth(userPath, 100);
    auth.setAuditService(&audit);

    // Setup initial admin
    const auto setupRes = auth.setupInitialAdmin("admin", "Admin#1234");
    assert(setupRes.success);

    // Register a teacher
    const Session adminSession{Role::Administrator, true, {}, "admin"};
    const auto regRes = auth.registerUser(adminSession, "teacher1", "Teach#1234", Role::Teacher);
    assert(regRes.success);

    // Login success
    const auto loginOk = auth.login("teacher1", "Teach#1234");
    assert(loginOk.success);

    // Failed login attempts causing account lockout
    auth.login("teacher1", "WrongPass1");
    auth.login("teacher1", "WrongPass2");
    auth.login("teacher1", "WrongPass3");
    assert(auth.isAccountLocked("teacher1"));

    // Unlock account
    const auto unlockRes = auth.unlockAccount(adminSession, "teacher1");
    assert(unlockRes.success);
    assert(!auth.isAccountLocked("teacher1"));

    // 2. StudentService integration
    StudentService students(studentPath);
    students.setAuditService(&audit);

    // Add student
    StudentRecord s1{"S201", "Charlie", "CS", "charlie@pes.edu"};
    assert(students.addStudent(adminSession, s1).success);

    // Update student
    assert(students.updateStudent(adminSession, "S201", "Charlie Brown", "CS", "cb@pes.edu").success);

    // Delete student
    assert(students.deleteStudent(adminSession, "S201", "Y").success);

    // Verify audit log has recorded all these events
    const auto allLogs = audit.query();
    assert(allLogs.size() >= 8);

    assert(audit.query("", "REGISTER_USER").size() == 1);
    assert(audit.query("", "LOGIN_SUCCESS").size() == 1);
    assert(audit.query("", "LOGIN_FAILED").size() == 3);
    assert(audit.query("", "ACCOUNT_LOCKED").size() == 1);
    assert(audit.query("", "ACCOUNT_UNLOCKED").size() == 1);
    assert(audit.query("", "CREATE_STUDENT").size() == 1);
    assert(audit.query("", "UPDATE_STUDENT").size() == 1);
    assert(audit.query("", "DELETE_STUDENT").size() == 1);

    std::filesystem::remove(auditPath);
    std::filesystem::remove(userPath);
    std::filesystem::remove(studentPath);
}

}  // namespace

int main() {
    testAuditLogLoginAndSecurityEvents();
    testAuditLogCRUDAndSystemOperations();
    testAppendOnlyIntegrityAndRetention();
    testLogInjectionPrevention();
    testMalformedRowHandling();
    testAuditWriteFailureAndFailSecurePolicy();
    testServiceIntegrationWithAuditLogging();
    return 0;
}

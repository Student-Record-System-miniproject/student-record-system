#include "srm/audit_service.h"
#include "srm/rbac.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

using srm::AccessController;
using srm::AuditEntry;
using srm::AuditService;
using srm::Operation;
using srm::Role;
using srm::SecurityEvent;
using srm::Session;

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
    assert(audit.logOperation(adminSession, "BACKUP_DATA", "backup_2026.tar", true, "Full system backup"));
    assert(audit.logOperation(adminSession, "RESTORE_DATA", "backup_2026.tar", true, "Restored state"));

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

}  // namespace

int main() {
    testAuditLogLoginAndSecurityEvents();
    testAuditLogCRUDAndSystemOperations();
    testAppendOnlyIntegrityAndRetention();
    return 0;
}

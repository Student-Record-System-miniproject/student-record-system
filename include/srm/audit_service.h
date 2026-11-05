#pragma once

#include "srm/common_types.h"
#include "srm/rbac.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace srm {

struct AuditEntry {
    std::string timestamp;        // ISO 8601 UTC timestamp: YYYY-MM-DDTHH:MM:SSZ
    std::string username;         // Actor who performed the action
    Role role;                    // Actor's role
    std::string action;           // e.g. LOGIN_SUCCESS, LOGIN_FAILED, ACCOUNT_LOCKED, ACCOUNT_UNLOCKED,
                                  // CREATE_STUDENT, UPDATE_STUDENT, DELETE_STUDENT, ENTER_MARKS,
                                  // SECURITY_VIOLATION, BACKUP_DATA, RESTORE_DATA
    std::string targetResource;   // Target roll number, subject, file, etc.
    std::string status;           // SUCCESS, FAILURE, DENIED
    std::string details;          // Additional contextual info
};

// SRM-F-044, SRM-NF-008, SRM-SR-005: Audit and security event logging service.
// Enforces an append-only audit trail at the application layer:
// - All system and security actions are logged with UTC timestamps and user information.
// - Entries are written in append-only mode (std::ios::app).
// - Input fields are sanitized to escape tabs and newlines to prevent log injection (CWE-117).
// - The service intentionally exposes NO API to edit, alter, or delete audit entries.
// - Malformed or corrupted lines during log inspection are tracked and reportable.
//
// NOTE ON PRODUCTION TAMPER RESISTANCE:
// While AuditService provides application-level append-only guarantees and guarantees that the
// application will never overwrite, truncate, or delete historical entries, operating-system level
// immutability (e.g. against root/administrator users modifying the underlying disk file) requires
// OS-level file attributes (chattr +a on Linux), dedicated WORM storage, or forwarding log streams
// to an external remote SIEM / syslog collector.
class AuditService {
public:
    explicit AuditService(std::filesystem::path logFilePath);

    // Append an entry to the append-only audit log
    bool record(const AuditEntry& entry);

    // Convenience logging helpers
    bool logLogin(const std::string& username, Role role, bool success, const std::string& details = "");
    bool logSecurityViolation(const SecurityEvent& event, const std::string& username = "");
    bool logOperation(const Session& session,
                      const std::string& action,
                      const std::string& targetResource,
                      bool success,
                      const std::string& details = "");

    // Read-only query / inspection of audit log entries (no mutation or deletion possible)
    std::vector<AuditEntry> query(const std::string& usernameFilter = "",
                                  const std::string& actionFilter = "") const;

    std::size_t count() const;
    std::size_t malformedRowsCount() const;
    const std::filesystem::path& logFilePath() const;

    // Field sanitization utilities to prevent log injection (CWE-117)
    static std::string sanitizeField(const std::string& input);
    static std::string unescapeField(const std::string& input);

private:
    std::filesystem::path logFilePath_;
    mutable std::size_t malformedRowsCount_ = 0;
    bool appendRow(const AuditEntry& entry) const;
    bool loadEntries(std::vector<AuditEntry>& out) const;
};

// Format current system time as UTC ISO 8601 string
std::string currentUtcTimestamp();

}  // namespace srm

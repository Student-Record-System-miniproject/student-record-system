#pragma once

#include "srm/grade_service.h"

#include <functional>
#include <string>
#include <vector>

namespace srm {

enum class Operation {
    // Administrator operations (SRM-SR-002, SRM-F-040, SRM-SR-003, SRM-F-012, SRM-F-013, SRM-F-041, SRM-F-042, SRM-F-044)
    ManageUsers,
    UnlockAccount,
    UpdateStudent,
    DeleteStudent,
    BackupData,
    RestoreData,
    ViewAuditLogs,

    // Teacher operations (SRM-SR-002, SRM-F-020, SRM-F-021, SRM-F-030, SRM-F-023, SRM-F-033, SRM-F-024)
    EnterMarks,
    UpdateMarks,
    MarkAttendance,
    ViewAnyStudentReportCard,
    ViewAnyStudentAttendanceReport,
    ExportAnyReport,

    // Student operations (SRM-SR-002, SRM-F-023, SRM-F-033, SRM-F-024, SRM-F-004)
    ViewOwnReportCard,
    ViewOwnAttendanceReport,
    ExportOwnReport,
    ChangeOwnPassword
};

struct SecurityEvent {
    Role role;
    Operation operation;
    std::string studentRollNumber;
    std::string targetResource;
    bool authorized;
    std::string reason;
};

using SecurityEventLogger = std::function<void(const SecurityEvent&)>;

class AccessController {
public:
    // Determines whether a given session is authorized for an operation.
    // Enforces:
    // - Unauthenticated sessions are always denied.
    // - Administrator can perform only Administrator operations.
    // - Teacher can perform only Teacher operations.
    // - Student can perform only Student operations, and only on their own roll number.
    static bool isAuthorized(const Session& session,
                            Operation operation,
                            const std::string& targetRollNumber = {});

    // Authorizes an operation. Returns success=true if permitted.
    // If forbidden, returns success=false with ErrorCode::AccessDenied,
    // and automatically logs the forbidden operation.
    static OperationResult authorize(const Session& session,
                                     Operation operation,
                                     const std::string& targetRollNumber = {});

    // Hook to attach external audit logging (for Issue #23 audit logger).
    static void setEventLogger(SecurityEventLogger logger);

    // Retrieve recorded security events.
    static const std::vector<SecurityEvent>& securityEvents();

    // Clear recorded security events (e.g. between tests).
    static void clearSecurityEvents();

private:
    static void logEvent(const SecurityEvent& event);
    static std::vector<SecurityEvent> events_;
    static SecurityEventLogger customLogger_;
};

}  // namespace srm

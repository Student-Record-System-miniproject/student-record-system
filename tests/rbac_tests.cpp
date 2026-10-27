#include "srm/rbac.h"

#include <cassert>
#include <string>
#include <vector>

namespace {

using srm::AccessController;
using srm::ErrorCode;
using srm::Operation;
using srm::Role;
using srm::SecurityEvent;
using srm::Session;

const Session adminSession{Role::Administrator, true, {}};
const Session teacherSession{Role::Teacher, true, {}};
const Session studentSession{Role::Student, true, "S001"};
const Session unauthenticatedSession{Role::Teacher, false, {}};

void testAdministratorPermissions() {
    AccessController::clearSecurityEvents();

    // Permitted administrator operations
    const std::vector<Operation> permittedAdminOps = {
        Operation::ManageUsers,
        Operation::UnlockAccount,
        Operation::UpdateStudent,
        Operation::DeleteStudent,
        Operation::BackupData,
        Operation::RestoreData,
        Operation::ViewAuditLogs
    };

    for (const auto op : permittedAdminOps) {
        assert(AccessController::isAuthorized(adminSession, op));
        const auto result = AccessController::authorize(adminSession, op);
        assert(result.success);
        assert(result.error == ErrorCode::None);
    }

    // Administrator forbidden from academic marks and attendance entry
    const std::vector<Operation> forbiddenAdminOps = {
        Operation::EnterMarks,
        Operation::UpdateMarks,
        Operation::MarkAttendance
    };

    for (const auto op : forbiddenAdminOps) {
        assert(!AccessController::isAuthorized(adminSession, op));
        const auto result = AccessController::authorize(adminSession, op);
        assert(!result.success);
        assert(result.error == ErrorCode::AccessDenied);
    }

    // Verify forbidden operations were recorded as security events
    assert(AccessController::securityEvents().size() == forbiddenAdminOps.size());
    for (const auto& event : AccessController::securityEvents()) {
        assert(event.role == Role::Administrator);
        assert(!event.authorized);
    }
}

void testTeacherPermissions() {
    AccessController::clearSecurityEvents();

    // Permitted teacher operations
    const std::vector<Operation> permittedTeacherOps = {
        Operation::EnterMarks,
        Operation::UpdateMarks,
        Operation::MarkAttendance,
        Operation::ViewAnyStudentReportCard,
        Operation::ViewAnyStudentAttendanceReport,
        Operation::ExportAnyReport
    };

    for (const auto op : permittedTeacherOps) {
        assert(AccessController::isAuthorized(teacherSession, op));
        const auto result = AccessController::authorize(teacherSession, op);
        assert(result.success);
        assert(result.error == ErrorCode::None);
    }

    // Teacher forbidden from user administration and system controls
    const std::vector<Operation> forbiddenTeacherOps = {
        Operation::ManageUsers,
        Operation::UnlockAccount,
        Operation::DeleteStudent,
        Operation::BackupData,
        Operation::RestoreData
    };

    for (const auto op : forbiddenTeacherOps) {
        assert(!AccessController::isAuthorized(teacherSession, op));
        const auto result = AccessController::authorize(teacherSession, op);
        assert(!result.success);
        assert(result.error == ErrorCode::AccessDenied);
    }

    assert(AccessController::securityEvents().size() == forbiddenTeacherOps.size());
}

void testStudentPermissions() {
    AccessController::clearSecurityEvents();

    // Student permitted to access own data
    assert(AccessController::isAuthorized(studentSession, Operation::ViewOwnReportCard, "S001"));
    assert(AccessController::isAuthorized(studentSession, Operation::ViewOwnAttendanceReport, "S001"));
    assert(AccessController::isAuthorized(studentSession, Operation::ExportOwnReport, "S001"));
    assert(AccessController::isAuthorized(studentSession, Operation::ChangeOwnPassword));

    assert(AccessController::authorize(studentSession, Operation::ViewOwnReportCard, "S001").success);
    assert(AccessController::authorize(studentSession, Operation::ViewOwnAttendanceReport, "S001").success);

    // Student forbidden from accessing peer's data (different roll number)
    assert(!AccessController::isAuthorized(studentSession, Operation::ViewOwnReportCard, "S002"));
    assert(!AccessController::isAuthorized(studentSession, Operation::ViewOwnAttendanceReport, "S002"));
    assert(!AccessController::isAuthorized(studentSession, Operation::ExportOwnReport, "S002"));

    const auto peerReportCheck = AccessController::authorize(
        studentSession, Operation::ViewOwnReportCard, "S002");
    assert(!peerReportCheck.success);
    assert(peerReportCheck.error == ErrorCode::AccessDenied);

    // Student forbidden from academic grading and administration
    const std::vector<Operation> forbiddenStudentOps = {
        Operation::EnterMarks,
        Operation::UpdateMarks,
        Operation::MarkAttendance,
        Operation::ManageUsers,
        Operation::DeleteStudent
    };

    for (const auto op : forbiddenStudentOps) {
        assert(!AccessController::isAuthorized(studentSession, op));
        const auto res = AccessController::authorize(studentSession, op);
        assert(!res.success);
        assert(res.error == ErrorCode::AccessDenied);
    }

    // Verify forbidden events were logged
    assert(!AccessController::securityEvents().empty());
}

void testUnauthenticatedAccessDenied() {
    AccessController::clearSecurityEvents();

    assert(!AccessController::isAuthorized(unauthenticatedSession, Operation::EnterMarks));
    const auto result = AccessController::authorize(unauthenticatedSession, Operation::EnterMarks);
    assert(!result.success);
    assert(result.error == ErrorCode::AccessDenied);

    const auto& events = AccessController::securityEvents();
    assert(events.size() == 1);
    assert(!events[0].authorized);
    assert(events[0].reason == "Unauthenticated session");
}

void testCustomSecurityEventLogger() {
    AccessController::clearSecurityEvents();

    std::vector<SecurityEvent> callbackReceived;
    AccessController::setEventLogger([&](const SecurityEvent& event) {
        callbackReceived.push_back(event);
    });

    AccessController::authorize(studentSession, Operation::EnterMarks);

    assert(callbackReceived.size() == 1);
    assert(callbackReceived[0].role == Role::Student);
    assert(callbackReceived[0].operation == Operation::EnterMarks);
    assert(!callbackReceived[0].authorized);

    // Reset logger hook to clean up
    AccessController::setEventLogger(nullptr);
}

}  // namespace

int main() {
    testAdministratorPermissions();
    testTeacherPermissions();
    testStudentPermissions();
    testUnauthenticatedAccessDenied();
    testCustomSecurityEventLogger();
    return 0;
}

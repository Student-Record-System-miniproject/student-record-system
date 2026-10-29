#include "srm/rbac.h"

#include <utility>

namespace srm {

std::vector<SecurityEvent> AccessController::events_;
SecurityEventLogger AccessController::customLogger_ = nullptr;

bool AccessController::isAuthorized(const Session& session,
                                    Operation operation,
                                    const std::string& targetRollNumber) {
    if (!session.authenticated) {
        return false;
    }

    switch (session.role) {
        case Role::Administrator:
            switch (operation) {
                case Operation::AddStudent:
                case Operation::UpdateStudent:
                case Operation::DeleteStudent:
                case Operation::RegisterUser:
                case Operation::ManageUsers:
                case Operation::UnlockAccount:
                case Operation::BackupData:
                case Operation::RestoreData:
                case Operation::ViewAuditLogs:
                    return true;
                default:
                    // Administrator cannot enter marks or modify academic records directly
                    return false;
            }

        case Role::Teacher:
            switch (operation) {
                case Operation::EnterMarks:
                case Operation::UpdateMarks:
                case Operation::MarkAttendance:
                case Operation::ViewAnyStudentReportCard:
                case Operation::ViewAnyStudentAttendanceReport:
                case Operation::ExportAnyReport:
                    return true;
                default:
                    // Teacher cannot perform administrative user or system management
                    return false;
            }

        case Role::Student:
            switch (operation) {
                case Operation::ViewOwnReportCard:
                case Operation::ViewOwnAttendanceReport:
                case Operation::ExportOwnReport:
                    // Student can access only their own roll number
                    return !session.studentRollNumber.empty() &&
                           session.studentRollNumber == targetRollNumber;
                case Operation::ChangeOwnPassword:
                    return true;
                default:
                    // Student cannot enter marks, view other students' records, or manage system
                    return false;
            }

        default:
            return false;
    }
}

OperationResult AccessController::authorize(const Session& session,
                                            Operation operation,
                                            const std::string& targetRollNumber) {
    if (!session.authenticated) {
        SecurityEvent event{session.role, operation, session.studentRollNumber,
                            targetRollNumber, false, "Unauthenticated session"};
        logEvent(event);
        return {false, ErrorCode::AccessDenied, "Authentication is required."};
    }

    if (!isAuthorized(session, operation, targetRollNumber)) {
        SecurityEvent event{session.role, operation, session.studentRollNumber,
                            targetRollNumber, false,
                            "Forbidden operation for role"};
        logEvent(event);
        return {false, ErrorCode::AccessDenied,
                "Access denied: Operation not authorized for current role."};
    }

    return {true, ErrorCode::None, "Authorized."};
}

void AccessController::setEventLogger(SecurityEventLogger logger) {
    customLogger_ = std::move(logger);
}

const std::vector<SecurityEvent>& AccessController::securityEvents() {
    return events_;
}

void AccessController::clearSecurityEvents() {
    events_.clear();
}

void AccessController::logEvent(const SecurityEvent& event) {
    events_.push_back(event);
    if (customLogger_) {
        customLogger_(event);
    }
}

}  // namespace srm

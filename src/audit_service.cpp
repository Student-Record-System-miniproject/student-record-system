#include "srm/audit_service.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace srm {

std::string currentUtcTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm utcTm{};
#if defined(_MSC_VER)
    gmtime_s(&utcTm, &tt);
#else
    gmtime_r(&tt, &utcTm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string AuditService::sanitizeField(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '\n': out += "\\n"; break;
            case '\\': out += "\\\\"; break;
            default:   out += c; break;
        }
    }
    return out;
}

std::string AuditService::unescapeField(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            if (next == 't') {
                out += '\t';
                ++i;
            } else if (next == 'r') {
                out += '\r';
                ++i;
            } else if (next == 'n') {
                out += '\n';
                ++i;
            } else if (next == '\\') {
                out += '\\';
                ++i;
            } else {
                out += '\\';
            }
        } else {
            out += input[i];
        }
    }
    return out;
}

AuditService::AuditService(std::filesystem::path logFilePath)
    : logFilePath_(std::move(logFilePath)) {}

bool AuditService::appendRow(const AuditEntry& entry) const {
    std::error_code ec;
    const auto parent = logFilePath_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            lastWriteFailed_ = true;
            ++writeFailureCount_;
            std::cerr << "[AUDIT ALERT] Unable to create audit directory: " << parent.string() << std::endl;
            return false;
        }
    }

    // Always opened strictly in append mode (SRM-NF-008: append-only audit trail)
    std::ofstream out(logFilePath_, std::ios::app);
    if (!out.is_open()) {
        lastWriteFailed_ = true;
        ++writeFailureCount_;
        std::cerr << "[AUDIT ALERT] Failed to open audit log for append: " << logFilePath_.string() << std::endl;
        return false;
    }

    // Sanitize every field to prevent log injection (CWE-117)
    out << sanitizeField(entry.timestamp.empty() ? currentUtcTimestamp() : entry.timestamp) << '\t'
        << sanitizeField(entry.username.empty() ? "anonymous" : entry.username) << '\t'
        << sanitizeField(roleToString(entry.role)) << '\t'
        << sanitizeField(entry.action) << '\t'
        << sanitizeField(entry.targetResource) << '\t'
        << sanitizeField(entry.status) << '\t'
        << sanitizeField(entry.details) << '\n';

    if (!out.good()) {
        lastWriteFailed_ = true;
        ++writeFailureCount_;
        std::cerr << "[AUDIT ALERT] Failed writing to audit log: " << logFilePath_.string() << std::endl;
        return false;
    }

    lastWriteFailed_ = false;
    return true;
}

bool AuditService::record(const AuditEntry& entry) {
    return appendRow(entry);
}

bool AuditService::logLogin(const std::string& username,
                            Role role,
                            bool success,
                            const std::string& details) {
    AuditEntry entry;
    entry.timestamp = currentUtcTimestamp();
    entry.username = username;
    entry.role = role;
    entry.action = success ? "LOGIN_SUCCESS" : "LOGIN_FAILED";
    entry.targetResource = "AUTH";
    entry.status = success ? "SUCCESS" : "FAILURE";
    entry.details = details;
    return record(entry);
}

bool AuditService::logSecurityViolation(const SecurityEvent& event,
                                        const std::string& username) {
    AuditEntry entry;
    entry.timestamp = currentUtcTimestamp();
    entry.username = !username.empty() ? username : (!event.username.empty() ? event.username : "anonymous");
    entry.role = event.role;
    entry.action = "SECURITY_VIOLATION";
    entry.targetResource = event.targetResource.empty() ? event.studentRollNumber : event.targetResource;
    entry.status = "DENIED";
    entry.details = event.reason;
    return record(entry);
}

bool AuditService::logOperation(const Session& session,
                                const std::string& action,
                                const std::string& targetResource,
                                bool success,
                                const std::string& details) {
    AuditEntry entry;
    entry.timestamp = currentUtcTimestamp();
    entry.username = session.username.empty() ? "system" : session.username;
    entry.role = session.role;
    entry.action = action;
    entry.targetResource = targetResource;
    entry.status = success ? "SUCCESS" : "FAILURE";
    entry.details = details;
    return record(entry);
}

bool AuditService::logBackup(const Session& session,
                             const std::string& backupArchive,
                             bool success,
                             const std::string& details) {
    return logOperation(session, "BACKUP_DATA", backupArchive, success, details);
}

bool AuditService::logRestore(const Session& session,
                              const std::string& backupArchive,
                              bool success,
                              const std::string& details) {
    return logOperation(session, "RESTORE_DATA", backupArchive, success, details);
}

bool AuditService::loadEntries(std::vector<AuditEntry>& out) const {
    out.clear();
    malformedRowsCount_ = 0;
    std::ifstream in(logFilePath_);
    if (!in.is_open()) {
        return !std::filesystem::exists(logFilePath_);
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream row(line);
        std::string rawTs, rawUser, rawRole, rawAction, rawTarget, rawStatus, rawDetails;

        if (!std::getline(row, rawTs, '\t') ||
            !std::getline(row, rawUser, '\t') ||
            !std::getline(row, rawRole, '\t') ||
            !std::getline(row, rawAction, '\t') ||
            !std::getline(row, rawTarget, '\t') ||
            !std::getline(row, rawStatus, '\t')) {
            ++malformedRowsCount_;
            continue;
        }

        std::getline(row, rawDetails);

        AuditEntry entry;
        entry.timestamp = unescapeField(rawTs);
        entry.username = unescapeField(rawUser);
        if (!stringToRole(unescapeField(rawRole), entry.role)) {
            ++malformedRowsCount_;
            continue;
        }
        entry.action = unescapeField(rawAction);
        entry.targetResource = unescapeField(rawTarget);
        entry.status = unescapeField(rawStatus);
        entry.details = unescapeField(rawDetails);

        out.push_back(std::move(entry));
    }

    return true;
}

std::vector<AuditEntry> AuditService::query(const std::string& usernameFilter,
                                            const std::string& actionFilter) const {
    std::vector<AuditEntry> all;
    loadEntries(all);

    std::vector<AuditEntry> filtered;
    for (const auto& entry : all) {
        if (!usernameFilter.empty() && entry.username != usernameFilter) {
            continue;
        }
        if (!actionFilter.empty() && entry.action != actionFilter) {
            continue;
        }
        filtered.push_back(entry);
    }
    return filtered;
}

std::size_t AuditService::count() const {
    std::vector<AuditEntry> all;
    loadEntries(all);
    return all.size();
}

std::size_t AuditService::malformedRowsCount() const {
    return malformedRowsCount_;
}

std::size_t AuditService::writeFailureCount() const {
    return writeFailureCount_;
}

bool AuditService::lastWriteFailed() const {
    return lastWriteFailed_;
}

const std::filesystem::path& AuditService::logFilePath() const {
    return logFilePath_;
}

}  // namespace srm

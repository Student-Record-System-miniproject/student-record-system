#include "srm/audit_service.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
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

AuditService::AuditService(std::filesystem::path logFilePath)
    : logFilePath_(std::move(logFilePath)) {}

bool AuditService::appendRow(const AuditEntry& entry) const {
    std::error_code ec;
    const auto parent = logFilePath_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }

    // Always opened strictly in append mode (SRM-NF-008: append-only audit trail)
    std::ofstream out(logFilePath_, std::ios::app);
    if (!out.is_open()) {
        return false;
    }

    out << (entry.timestamp.empty() ? currentUtcTimestamp() : entry.timestamp) << '\t'
        << (entry.username.empty() ? "anonymous" : entry.username) << '\t'
        << roleToString(entry.role) << '\t'
        << entry.action << '\t'
        << entry.targetResource << '\t'
        << entry.status << '\t'
        << entry.details << '\n';

    return out.good();
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
    entry.username = username.empty() ? "anonymous" : username;
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

bool AuditService::loadEntries(std::vector<AuditEntry>& out) const {
    out.clear();
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
        AuditEntry entry;
        std::string roleStr;

        if (!std::getline(row, entry.timestamp, '\t') ||
            !std::getline(row, entry.username, '\t') ||
            !std::getline(row, roleStr, '\t') ||
            !std::getline(row, entry.action, '\t') ||
            !std::getline(row, entry.targetResource, '\t') ||
            !std::getline(row, entry.status, '\t')) {
            continue;
        }

        stringToRole(roleStr, entry.role);
        std::getline(row, entry.details);

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

const std::filesystem::path& AuditService::logFilePath() const {
    return logFilePath_;
}

}  // namespace srm

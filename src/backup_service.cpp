
#include "srm/backup_service.h"

#include "srm/audit_service.h"
#include "srm/rbac.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace srm {

BackupService::BackupService(
    std::filesystem::path dataDirectory,
    std::filesystem::path backupDirectory,
    AuditService* auditService)
    : dataDirectory_(std::move(dataDirectory)),
      backupDirectory_(std::move(backupDirectory)),
      auditService_(auditService) {}

void BackupService::setAuditService(AuditService* auditService) {
    auditService_ = auditService;
}

std::string BackupService::generateBackupName() const {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostringstream name;
    name << "backup_" << std::put_time(&localTime, "%Y%m%d_%H%M%S");

    return name.str();
}

std::vector<std::filesystem::path> BackupService::listBackups() const {
    std::vector<std::filesystem::path> backups;

    std::error_code error;

    if (!std::filesystem::exists(backupDirectory_, error) || error) {
        return backups;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(backupDirectory_, error)) {
        if (error) {
            break;
        }

        if (entry.is_directory(error) && !error) {
            backups.push_back(entry.path());
        }
    }

    std::sort(backups.begin(), backups.end());

    return backups;
}

bool BackupService::validateBackup(
    const std::filesystem::path& backupPath) const {

    std::error_code error;

    if (!std::filesystem::is_directory(backupPath, error) || error) {
        return false;
    }

    bool containsAtLeastOneFile = false;

    for (const auto& entry :
         std::filesystem::directory_iterator(backupPath, error)) {
        if (error) {
            return false;
        }

        if (!entry.is_regular_file(error) || error) {
            return false;
        }

        containsAtLeastOneFile = true;
    }

    return containsAtLeastOneFile;
}

OperationResult BackupService::createBackup(const Session& session) {
    if (!AccessController::isAuthorized(session, Operation::BackupData)) {
        return {false, ErrorCode::AccessDenied,
                "Only administrators can create backups."};
    }

    std::error_code error;

    std::filesystem::create_directories(dataDirectory_, error);
    if (error) {
        return {false, ErrorCode::StorageFailure,
                "Could not access the data directory."};
    }

    std::filesystem::create_directories(backupDirectory_, error);
    if (error) {
        return {false, ErrorCode::StorageFailure,
                "Could not create the backup directory."};
    }

    const std::string backupName = generateBackupName();
    const auto backupPath = backupDirectory_ / backupName;

    if (!std::filesystem::create_directory(backupPath, error) || error) {
        return {false, ErrorCode::StorageFailure,
                "Could not create the backup folder."};
    }

    bool success = true;

    for (const auto& entry :
         std::filesystem::directory_iterator(dataDirectory_, error)) {
        if (error) {
            success = false;
            break;
        }

        if (!entry.is_regular_file(error) || error) {
            continue;
        }

        const auto destination = backupPath / entry.path().filename();

        std::filesystem::copy_file(
            entry.path(),
            destination,
            std::filesystem::copy_options::overwrite_existing,
            error);

        if (error) {
            success = false;
            break;
        }
    }

    if (success) {
        success = validateBackup(backupPath);
    }

    if (!success) {
        std::filesystem::remove_all(backupPath, error);

        if (auditService_) {
            auditService_->logBackup(
                session, backupName, false, "Backup creation failed.");
        }

        return {false, ErrorCode::StorageFailure,
                "Backup creation failed."};
    }

    if (auditService_) {
        auditService_->logBackup(
            session, backupName, true, "Backup created successfully.");
    }

    return {true, ErrorCode::None,
            "Backup created successfully: " + backupName};
}

OperationResult BackupService::restoreBackup(
    const Session& session,
    const std::string& backupName,
    const std::string& confirmation) {

    if (!AccessController::isAuthorized(session, Operation::RestoreData)) {
        return {false, ErrorCode::AccessDenied,
                "Only administrators can restore backups."};
    }

    if (confirmation != "Y" && confirmation != "y") {
        return {false, ErrorCode::InvalidInput,
                "Restore cancelled. Confirmation was not provided."};
    }

    const std::filesystem::path namePath(backupName);

    if (backupName.empty() || namePath.has_parent_path() ||
        namePath.filename() != namePath) {
        return {false, ErrorCode::InvalidInput,
                "Invalid backup name."};
    }

    const auto backupPath = backupDirectory_ / namePath;

    if (!validateBackup(backupPath)) {
        return {false, ErrorCode::InvalidInput,
                "The selected backup is missing or invalid."};
    }

    std::error_code error;

    std::filesystem::create_directories(dataDirectory_, error);
    if (error) {
        return {false, ErrorCode::StorageFailure,
                "Could not access the data directory."};
    }

    // Preserve the current data in case restoration fails.
    const auto rollbackPath =
        backupDirectory_ / (backupName + "_rollback");

    std::filesystem::remove_all(rollbackPath, error);
    error.clear();

    std::filesystem::create_directory(rollbackPath, error);
    if (error) {
        return {false, ErrorCode::StorageFailure,
                "Could not prepare restore rollback."};
    }

    // Copy current data files for rollback, excluding the live audit log.
    for (const auto& entry :
         std::filesystem::directory_iterator(dataDirectory_, error)) {
        if (error) {
            break;
        }

        if (!entry.is_regular_file(error) || error) {
            continue;
        }

        if (entry.path().filename() == "audit.log") {
            continue;
        }

        std::filesystem::copy_file(
            entry.path(),
            rollbackPath / entry.path().filename(),
            std::filesystem::copy_options::overwrite_existing,
            error);

        if (error) {
            break;
        }
    }

    if (error) {
        std::filesystem::remove_all(rollbackPath, error);
        return {false, ErrorCode::StorageFailure,
                "Could not prepare rollback data."};
    }

    bool success = true;

    // Remove current data files that are not present in the backup.
    for (const auto& entry :
         std::filesystem::directory_iterator(dataDirectory_, error)) {
        if (error) {
            success = false;
            break;
        }

        if (!entry.is_regular_file(error) || error) {
            continue;
        }

        if (entry.path().filename() == "audit.log") {
            continue;
        }

        if (!std::filesystem::exists(
                backupPath / entry.path().filename(), error)) {
            if (error || !std::filesystem::remove(entry.path(), error) ||
                error) {
                success = false;
                break;
            }
        }
    }

    // Restore files from the selected snapshot.
    if (success) {
        for (const auto& entry :
             std::filesystem::directory_iterator(backupPath, error)) {
            if (error) {
                success = false;
                break;
            }

            if (!entry.is_regular_file(error) || error) {
                success = false;
                break;
            }

            if (entry.path().filename() == "audit.log") {
                continue;
            }

            std::filesystem::copy_file(
                entry.path(),
                dataDirectory_ / entry.path().filename(),
                std::filesystem::copy_options::overwrite_existing,
                error);

            if (error) {
                success = false;
                break;
            }
        }
    }

    // If restoration fails, put the original data back.
    if (!success) {
        for (const auto& entry :
             std::filesystem::directory_iterator(dataDirectory_, error)) {
            if (!entry.is_regular_file(error) || error) {
                continue;
            }

            if (entry.path().filename() != "audit.log") {
                std::filesystem::remove(entry.path(), error);
            }
        }

        error.clear();

        for (const auto& entry :
             std::filesystem::directory_iterator(rollbackPath, error)) {
            if (error) {
                break;
            }

            std::filesystem::copy_file(
                entry.path(),
                dataDirectory_ / entry.path().filename(),
                std::filesystem::copy_options::overwrite_existing,
                error);
        }

        std::filesystem::remove_all(rollbackPath, error);

        if (auditService_) {
            auditService_->logRestore(
                session, backupName, false, "Restore failed; rollback attempted.");
        }

        return {false, ErrorCode::StorageFailure,
                "Restore failed. Original data rollback was attempted."};
    }

    std::filesystem::remove_all(rollbackPath, error);

    if (auditService_) {
        auditService_->logRestore(
            session, backupName, true, "Data restored successfully.");
    }

    return {true, ErrorCode::None,
            "Backup restored successfully: " + backupName};
}

}  // namespace srm

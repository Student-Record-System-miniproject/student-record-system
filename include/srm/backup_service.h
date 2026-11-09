
#pragma once

#include "srm/common_types.h"

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

class AuditService;

class BackupService {
public:
    BackupService(std::filesystem::path dataDirectory,
                  std::filesystem::path backupDirectory,
                  AuditService* auditService = nullptr);

    // SRM-F-041: Create a timestamped backup of system data.
    OperationResult createBackup(const Session& session);

    // List available backup directories.
    std::vector<std::filesystem::path> listBackups() const;

    // SRM-F-042: Restore a selected backup after explicit confirmation.
    OperationResult restoreBackup(const Session& session,
                                  const std::string& backupName,
                                  const std::string& confirmation);

    void setAuditService(AuditService* auditService);

private:
    std::filesystem::path dataDirectory_;
    std::filesystem::path backupDirectory_;
    AuditService* auditService_;

    std::string generateBackupName() const;
    bool validateBackup(const std::filesystem::path& backupPath) const;
};

}  // namespace srm

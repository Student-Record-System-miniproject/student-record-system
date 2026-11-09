#include "srm/backup_service.h"
#include "srm/common_types.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

void writeFile(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    assert(file.is_open());
    file << content;
}

std::string readFile(const fs::path& path) {
    std::ifstream file(path);
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

int main() {
    const auto uniqueId =
        std::chrono::steady_clock::now()
            .time_since_epoch().count();

    const fs::path root =
        fs::temp_directory_path() /
        ("srm_backup_test_" + std::to_string(uniqueId));

    const fs::path dataDir = root / "data";
    const fs::path backupDir = root / "backups";

    fs::create_directories(dataDir);

    writeFile(dataDir / "students.txt", "STUDENT_OLD");
    writeFile(dataDir / "users.txt", "USER_OLD");

    srm::Session admin;
    admin.role = srm::Role::Administrator;
    admin.authenticated = true;
    admin.username = "admin";

    srm::Session student;
    student.role = srm::Role::Student;
    student.authenticated = true;
    student.username = "student";

    srm::BackupService backupService(dataDir, backupDir);

    // Test 1: Administrator can create a backup.
    auto backupResult = backupService.createBackup(admin);
    assert(backupResult.success);

    auto backups = backupService.listBackups();
    assert(!backups.empty());

    const std::string backupName =
        backups.back().filename().string();

    // Test 2: Backup contains all data files.
    assert(fs::exists(backups.back() / "students.txt"));
    assert(fs::exists(backups.back() / "users.txt"));

    assert(readFile(backups.back() / "students.txt") == "STUDENT_OLD");
    assert(readFile(backups.back() / "users.txt") == "USER_OLD");

    // Test 3: Non-admin cannot create a backup.
    auto deniedBackup = backupService.createBackup(student);
    assert(!deniedBackup.success);
    assert(deniedBackup.error == srm::ErrorCode::AccessDenied);

    // Test 4: Restore without confirmation is rejected.
    auto cancelledRestore =
        backupService.restoreBackup(admin, backupName, "N");

    assert(!cancelledRestore.success);
    assert(cancelledRestore.error == srm::ErrorCode::InvalidInput);

    // Test 5: Modify current data after taking the snapshot.
    writeFile(dataDir / "students.txt", "STUDENT_NEW");
    writeFile(dataDir / "extra.txt", "EXTRA_DATA");

    // Test 6: Administrator can restore the backup.
    auto restoreResult =
        backupService.restoreBackup(admin, backupName, "Y");

    assert(restoreResult.success);

    // Test 7: Restored records match the original snapshot.
    assert(readFile(dataDir / "students.txt") == "STUDENT_OLD");
    assert(readFile(dataDir / "users.txt") == "USER_OLD");

    // Test 8: Files created after backup are removed.
    assert(!fs::exists(dataDir / "extra.txt"));

    // Test 9: Non-admin cannot restore a backup.
    auto deniedRestore =
        backupService.restoreBackup(student, backupName, "Y");

    assert(!deniedRestore.success);
    assert(deniedRestore.error == srm::ErrorCode::AccessDenied);

    fs::remove_all(root);

    std::cout << "All backup service tests passed!\n";
    return 0;
}
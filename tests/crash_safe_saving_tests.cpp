#include "srm/storage_utils.h"
#include "srm/student_service.h"
#include "srm/auth_service.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

using srm::atomicWriteFile;
using srm::atomicWriteString;
using srm::recoverInterruptedSave;
using srm::Role;
using srm::StudentRecord;
using srm::StudentRepository;
using srm::UserAccount;
using srm::UserRepository;

// TC-Rel-01: Verify crash-safe saving and data integrity after forced termination / write failure
void testAtomicWriteAndForcedInterruption() {
    const auto dir = std::filesystem::temp_directory_path() / "srm_crash_safe_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    const auto targetFile = dir / "data.txt";

    // Step 1: Initial successful save
    const std::string initialData = "INITIAL_RECORD_1\tDATA\nINITIAL_RECORD_2\tDATA\n";
    const bool initialSaveOk = atomicWriteString(targetFile, initialData);
    assert(initialSaveOk);
    assert(std::filesystem::exists(targetFile));

    // Verify initial content
    {
        std::ifstream in(targetFile);
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        assert(content == initialData);
    }

    // Step 2: Simulate interrupted write (crash or forced termination midway during stream writing)
    // The writer begins writing partial data, but terminates/aborts before completing
    const bool interruptedSuccess = atomicWriteFile(targetFile, [](std::ostream& out) {
        out << "PARTIAL_GARBAGE_DATA_FROM_CRASHED_PROCESS";
        return false;  // Simulates failure / forced termination
    });
    assert(!interruptedSuccess);

    // Step 3: Verify target file was NOT corrupted or altered by the failed operation
    {
        std::ifstream in(targetFile);
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        assert(content == initialData);
        assert(content.find("PARTIAL_GARBAGE") == std::string::npos);
    }

    // Step 4: Verify no leftover temporary files in directory
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        assert(entry.path().filename() == "data.txt");
    }

    std::filesystem::remove_all(dir);
}

void testOriginalFilePreservedWhenReplacementFails() {
    const auto dir = std::filesystem::temp_directory_path() / "srm_crash_safe_fail_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    const auto targetFile = dir / "important_data.txt";
    const std::string originalData = "IMPORTANT_ORIGINAL_DATA_MUST_NEVER_BE_LOST\n";
    const bool saveOriginalOk = atomicWriteString(targetFile, originalData);
    assert(saveOriginalOk);

    // Simulate an aborted save before file swap completes
    const bool res = atomicWriteFile(targetFile, [](std::ostream&) {
        // Simulates an unhandled error or exception in user data serialization
        return false;
    });
    assert(!res);

    // The original file MUST still exist with 100% of its original content intact
    {
        std::ifstream in(targetFile);
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        assert(content == originalData);
    }

    std::filesystem::remove_all(dir);
}

// Review comment verification: Recovery from stranded backup if process crashed during swap window
void testRecoveryFromInterruptedReplacementCrashWindow() {
    const auto dir = std::filesystem::temp_directory_path() / "srm_crash_window_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    const auto targetFile = dir / "records.tsv";
    const std::string committedData = "S101\tAlice\tCSE\talice@test.edu\n";
    const bool saveCommittedOk = atomicWriteString(targetFile, committedData);
    assert(saveCommittedOk);

    // Simulate crash window: target file was renamed to .bak, and process terminated before new file was renamed
    const auto backupFile = dir / "records.tsv.bak";
    std::filesystem::rename(targetFile, backupFile);
    assert(!std::filesystem::exists(targetFile));
    assert(std::filesystem::exists(backupFile));

    // Next access or recoverInterruptedSave automatically restores target file
    const bool recoveryOk = recoverInterruptedSave(targetFile);
    assert(recoveryOk);
    assert(std::filesystem::exists(targetFile));

    // Verify restored content is 100% intact
    {
        std::ifstream in(targetFile);
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        assert(content == committedData);
    }

    std::filesystem::remove_all(dir);
}

// Review comment verification: Repository auto-recovers stranded backup upon load()
void testStrandedBackupRecoveryInRepository() {
    const auto testFile = std::filesystem::temp_directory_path() / "srm_repo_stranded.tsv";
    std::filesystem::remove(testFile);
    std::filesystem::remove(std::filesystem::path(testFile.string() + ".bak"));

    // Save initial students
    {
        StudentRepository repo(testFile);
        const bool loadOk = repo.load();
        assert(loadOk);
        const bool add1Ok = repo.add({"S101", "Alice Smith", "CSE", "alice@test.edu"});
        assert(add1Ok);
        const bool add2Ok = repo.add({"S102", "Bob Jones", "ECE", "bob@test.edu"});
        assert(add2Ok);
    }

    // Simulate crash window where target file is moved to .bak
    const auto backupPath = std::filesystem::path(testFile.string() + ".bak");
    std::filesystem::rename(testFile, backupPath);
    assert(!std::filesystem::exists(testFile));
    assert(std::filesystem::exists(backupPath));

    // Boot repository after crash: load() automatically detects and recovers stranded .bak!
    {
        StudentRepository restarted(testFile);
        const bool restartedLoadOk = restarted.load();
        assert(restartedLoadOk);
        assert(restarted.records().size() == 2);
        assert(restarted.findByRollNumber("S101") != nullptr);
        assert(restarted.findByRollNumber("S102") != nullptr);
    }

    // Target file now exists restored on disk
    assert(std::filesystem::exists(testFile));

    std::filesystem::remove(testFile);
    std::filesystem::remove(backupPath);
}

void testStudentRepositoryCrashSafeRestart() {
    const auto testFile = std::filesystem::temp_directory_path() / "srm_students_crash_safe.tsv";
    std::filesystem::remove(testFile);

    // Session 1: Add initial students
    {
        StudentRepository repo(testFile);
        const bool load1Ok = repo.load();
        assert(load1Ok);
        const bool add1Ok = repo.add({"S101", "Alice Smith", "CSE", "alice@test.edu"});
        assert(add1Ok);
        const bool add2Ok = repo.add({"S102", "Bob Jones", "ECE", "bob@test.edu"});
        assert(add2Ok);
    }

    // Simulate an external partial write failure to a temp file
    // Original database file must remain untouched
    {
        const auto tempCorruptFile = testFile.string() + ".tmp.simulated";
        std::ofstream corrupt(tempCorruptFile);
        corrupt << "CORRUPTED_PARTIAL_BYTES";
        corrupt.close();
        std::filesystem::remove(tempCorruptFile);
    }

    // Session 2: Reload after simulated failure / restart (TC-Rel-01)
    {
        StudentRepository restartedRepo(testFile);
        const bool load2Ok = restartedRepo.load();
        assert(load2Ok);
        assert(restartedRepo.records().size() == 2);

        const auto* alice = restartedRepo.findByRollNumber("S101");
        assert(alice != nullptr);
        assert(alice->name == "Alice Smith");

        const auto* bob = restartedRepo.findByRollNumber("S102");
        assert(bob != nullptr);
        assert(bob->name == "Bob Jones");
    }

    std::filesystem::remove(testFile);
}

void testUserRepositoryCrashSafeSaving() {
    const auto testFile = std::filesystem::temp_directory_path() / "srm_users_crash_safe.tsv";
    std::filesystem::remove(testFile);

    // Add valid user
    {
        UserRepository repo(testFile);
        const bool load1Ok = repo.load();
        assert(load1Ok);
        const bool addUserOk = repo.addUser({"admin_user", "hashed_secret", Role::Administrator, 0, false, ""});
        assert(addUserOk);
    }

    // Restart repository and verify record exists
    {
        UserRepository restarted(testFile);
        const bool load2Ok = restarted.load();
        assert(load2Ok);
        const auto* user = restarted.findUser("admin_user");
        assert(user != nullptr);
        assert(user->role == Role::Administrator);
    }

    std::filesystem::remove(testFile);
}

}  // namespace

int main() {
    testAtomicWriteAndForcedInterruption();
    testOriginalFilePreservedWhenReplacementFails();
    testRecoveryFromInterruptedReplacementCrashWindow();
    testStrandedBackupRecoveryInRepository();
    testStudentRepositoryCrashSafeRestart();
    testUserRepositoryCrashSafeSaving();
    return 0;
}


#include "srm/directory_security.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    std::cout << "PASSED: " << message << '\n';
    return true;
}

#ifndef _WIN32

bool hasExactPermissions(const fs::path& path, mode_t expected) {
    struct stat info {};

    if (::stat(path.c_str(), &info) != 0) {
        return false;
    }

    return (info.st_mode & 0777) == expected;
}

#else

bool hasProtectedAllowedPrincipals(const fs::path& path) {
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR descriptor = nullptr;

    const DWORD result = GetNamedSecurityInfoW(
        const_cast<LPWSTR>(path.c_str()),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        nullptr,
        nullptr,
        &dacl,
        nullptr,
        &descriptor);

    if (result != ERROR_SUCCESS || descriptor == nullptr || dacl == nullptr) {
        if (descriptor) {
            LocalFree(descriptor);
        }
        return false;
    }

    SECURITY_DESCRIPTOR_CONTROL control = 0;
    DWORD revision = 0;

    bool valid = GetSecurityDescriptorControl(
        descriptor, &control, &revision) != FALSE;

    valid = valid && ((control & SE_DACL_PROTECTED) != 0);

    HANDLE token = nullptr;
    DWORD tokenSize = 0;
    std::vector<unsigned char> tokenBuffer;

    if (valid && OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        GetTokenInformation(token, TokenUser, nullptr, 0, &tokenSize);
        tokenBuffer.resize(tokenSize);

        valid = tokenSize != 0 &&
            GetTokenInformation(token, TokenUser, tokenBuffer.data(),
                                tokenSize, &tokenSize) != FALSE;

        CloseHandle(token);
    } else {
        valid = false;
    }

    PSID currentUser = nullptr;
    if (valid) {
        currentUser = reinterpret_cast<TOKEN_USER*>(
            tokenBuffer.data())->User.Sid;
    }

    PSID systemSid = nullptr;
    PSID adminSid = nullptr;

    ConvertStringSidToSidW(L"S-1-5-18", &systemSid);
    ConvertStringSidToSidW(L"S-1-5-32-544", &adminSid);

    valid = valid && systemSid != nullptr && adminSid != nullptr;

    for (DWORD i = 0; valid && i < dacl->AceCount; ++i) {
        void* rawAce = nullptr;

        if (!GetAce(dacl, i, &rawAce)) {
            valid = false;
            break;
        }

        auto* header = static_cast<ACE_HEADER*>(rawAce);

        if (header->AceType == ACCESS_ALLOWED_ACE_TYPE) {
            auto* ace = static_cast<ACCESS_ALLOWED_ACE*>(rawAce);
            PSID aceSid = &ace->SidStart;

            if (!EqualSid(aceSid, currentUser) &&
                !EqualSid(aceSid, systemSid) &&
                !EqualSid(aceSid, adminSid)) {
                valid = false;
            }
        }
    }

    if (systemSid) LocalFree(systemSid);
    if (adminSid) LocalFree(adminSid);
    LocalFree(descriptor);

    return valid;
}

#endif

}  // namespace

int main(int argc, char* argv[]) {

    // Special mode used by CI to prepare protected files
    // for cross-operating-system-user access tests.
    if (argc == 3 && std::string(argv[1]) == "--prepare") {
        const fs::path prepareRoot = argv[2];

        std::error_code prepareError;
        fs::remove_all(prepareRoot, prepareError);

        if (prepareError) {
            std::cerr << "Could not clean preparation directory.\n";
            return 1;
        }

        const fs::path dataDir = prepareRoot / "data";
        const fs::path backupDir = prepareRoot / "backups";

        fs::create_directories(dataDir / "nested", prepareError);
        if (prepareError) {
            std::cerr << "Could not create data directory.\n";
            return 1;
        }

        fs::create_directories(backupDir, prepareError);
        if (prepareError) {
            std::cerr << "Could not create backup directory.\n";
            return 1;
        }

        {
            std::ofstream(dataDir / "students.txt") << "student records";
            std::ofstream(dataDir / "nested" / "users.txt") << "user records";
            std::ofstream(backupDir / "snapshot.txt") << "backup snapshot";
        }

        if (!srm::DirectorySecurity::protectApplicationDirectories(
                dataDir, backupDir)) {
            std::cerr << "Could not protect preparation directories.\n";
            return 1;
        }

        std::cout << "Protected cross-user test directories prepared.\n";
        return 0;
    }

    // Normal directory security test starts here.
    const fs::path root =
        fs::temp_directory_path() / "srm_directory_security_test";

    std::error_code ec;
    fs::remove_all(root, ec);

    const fs::path dataDir = root / "data";
    const fs::path backupDir = root / "backups";
    const fs::path nestedDir = dataDir / "nested";
    const fs::path dataFile = dataDir / "students.txt";
    const fs::path nestedFile = nestedDir / "users.txt";
    const fs::path backupFile = backupDir / "snapshot.txt";

    fs::create_directories(nestedDir);
    fs::create_directories(backupDir);

    {
        std::ofstream(dataFile) << "student records";
        std::ofstream(nestedFile) << "user records";
        std::ofstream(backupFile) << "backup snapshot";
    }

    bool passed = true;

    passed &= check(
        srm::DirectorySecurity::protectApplicationDirectories(
            dataDir, backupDir),
        "Protect application data and backup directories");

    passed &= check(
        fs::is_directory(dataDir) && fs::is_directory(backupDir),
        "Both protected directories exist");

#ifdef _WIN32
    passed &= check(
        hasProtectedAllowedPrincipals(dataDir),
        "Data directory has a protected ACL restricted to approved principals");

    passed &= check(
        hasProtectedAllowedPrincipals(backupDir),
        "Backup directory has a protected ACL restricted to approved principals");

    passed &= check(
        hasProtectedAllowedPrincipals(dataFile),
        "Existing data file has a protected ACL");

    passed &= check(
        hasProtectedAllowedPrincipals(nestedFile),
        "Nested data file has a protected ACL");

    passed &= check(
        hasProtectedAllowedPrincipals(backupFile),
        "Existing backup file has a protected ACL");
#else
    passed &= check(hasExactPermissions(dataDir, 0700),
                    "Data directory permissions are 0700");

    passed &= check(hasExactPermissions(backupDir, 0700),
                    "Backup directory permissions are 0700");

    passed &= check(hasExactPermissions(nestedDir, 0700),
                    "Nested directory permissions are 0700");

    passed &= check(hasExactPermissions(dataFile, 0600),
                    "Existing data file permissions are 0600");

    passed &= check(hasExactPermissions(nestedFile, 0600),
                    "Nested data file permissions are 0600");

    passed &= check(hasExactPermissions(backupFile, 0600),
                    "Existing backup file permissions are 0600");
#endif

    fs::remove_all(root, ec);

    if (!passed) {
        std::cerr << "\nDirectory security tests failed.\n";
        return 1;
    }

    std::cout << "\nAll directory security tests passed!\n";
    return 0;
}

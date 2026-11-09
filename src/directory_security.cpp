#include "srm/directory_security.h"

#include <filesystem>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sddl.h>
#include <memory>
#include <string>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace srm {
namespace {

#ifdef _WIN32

bool applyWindowsProtection(const std::filesystem::path& path) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }

    DWORD requiredSize = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &requiredSize);

    if (requiredSize == 0) {
        CloseHandle(token);
        return false;
    }

    auto tokenBuffer = std::make_unique<unsigned char[]>(requiredSize);

    if (!GetTokenInformation(token, TokenUser, tokenBuffer.get(),
                             requiredSize, &requiredSize)) {
        CloseHandle(token);
        return false;
    }

    CloseHandle(token);

    const auto* tokenUser =
        reinterpret_cast<const TOKEN_USER*>(tokenBuffer.get());

    LPWSTR userSidString = nullptr;

    if (!ConvertSidToStringSidW(tokenUser->User.Sid, &userSidString)) {
        return false;
    }

    // Allow only the current user, SYSTEM and local Administrators.
    // OI/CI propagate permissions to child files and directories.
    const std::wstring sddl =
        L"D:P(A;OICI;FA;;;" + std::wstring(userSidString) +
        L")(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)";

    LocalFree(userSidString);

    PSECURITY_DESCRIPTOR descriptor = nullptr;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) {
        return false;
    }

    const BOOL result = SetFileSecurityW(
        path.c_str(),
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        descriptor);

    LocalFree(descriptor);

    return result != FALSE;
}

#else

bool applyPosixProtection(const std::filesystem::path& path) {
    struct stat info {};

    if (::lstat(path.c_str(), &info) != 0 || S_ISLNK(info.st_mode)) {
        return false;
    }

    mode_t permissions = 0;

    if (S_ISDIR(info.st_mode)) {
        permissions = S_IRWXU;  // 0700
    }
    else if (S_ISREG(info.st_mode)) {
        permissions = S_IRUSR | S_IWUSR;  // 0600
    }
    else {
        return false;
    }

    return ::chmod(path.c_str(), permissions) == 0;
}

#endif

bool applyProtection(const std::filesystem::path& path) {
#ifdef _WIN32
    return applyWindowsProtection(path);
#else
    return applyPosixProtection(path);
#endif
}

}  // namespace

bool DirectorySecurity::protectDirectory(
    const std::filesystem::path& directory) {

    if (directory.empty()) {
        return false;
    }

    std::error_code ec;

    std::filesystem::create_directories(directory, ec);

    if (ec || !std::filesystem::is_directory(directory, ec) || ec) {
        return false;
    }

    if (!applyProtection(directory)) {
        return false;
    }

    std::filesystem::recursive_directory_iterator it(
        directory, std::filesystem::directory_options::none, ec);

    const std::filesystem::recursive_directory_iterator end;

    if (ec) {
        return false;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            return false;
        }

        const auto status = it->symlink_status(ec);

        if (ec || std::filesystem::is_symlink(status)) {
            return false;
        }

        if (!applyProtection(it->path())) {
            return false;
        }
    }

    return !ec;
}

bool DirectorySecurity::protectApplicationDirectories(
    const std::filesystem::path& dataDirectory,
    const std::filesystem::path& backupDirectory) {

    if (!protectDirectory(dataDirectory)) {
        return false;
    }

    return protectDirectory(backupDirectory);
}

}  // namespace srm

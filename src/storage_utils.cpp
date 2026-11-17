#include "srm/storage_utils.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <system_error>

namespace srm {
namespace {

std::atomic<std::uint64_t> g_atomicSequence{0};

std::string generateUniqueTempPath(const std::filesystem::path& targetFilePath) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto seq = ++g_atomicSequence;
    std::random_device rd;
    const auto rnd = rd();
    return targetFilePath.string() + ".tmp." + std::to_string(now) + "." +
           std::to_string(seq) + "." + std::to_string(rnd);
}

bool restoreBackupFile(
    const std::filesystem::path& backup,
    const std::filesystem::path& target) {

    std::error_code ec;

    std::filesystem::rename(backup, target, ec);
    if (!ec) {
        return true;
    }

    std::filesystem::copy_file(
        backup, target,
        std::filesystem::copy_options::overwrite_existing, ec);

    if (!ec) {
        std::error_code removeError;
        std::filesystem::remove(backup, removeError);
        return true;
    }

    return false;
}

std::filesystem::path findTimestampedBackup(
    const std::filesystem::path& target,
    std::error_code& ec) {

    const auto parent = target.parent_path();

    if (parent.empty() || !std::filesystem::exists(parent, ec)) {
        return {};
    }

    const std::string prefix = target.filename().string() + ".bak.";

    for (const auto& entry : std::filesystem::directory_iterator(parent, ec)) {
        if (entry.path().filename().string().rfind(prefix, 0) == 0) {
            return entry.path();
        }
    }

    return {};
}

bool writeTemporaryFile(
    const std::filesystem::path& tempPath,
    const std::function<bool(std::ostream&)>& writeCallback) {

    std::error_code ec;
    std::ofstream tempStream(tempPath, std::ios::trunc);

    if (!tempStream.is_open()) {
        return false;
    }

    const bool writeOk = writeCallback(tempStream);

    if (!writeOk || !tempStream.good()) {
        tempStream.close();
        std::filesystem::remove(tempPath, ec);
        return false;
    }

    tempStream.flush();

    if (!tempStream.good()) {
        tempStream.close();
        std::filesystem::remove(tempPath, ec);
        return false;
    }

    return true;
}

bool createTargetBackup(
    const std::filesystem::path& targetPath,
    const std::filesystem::path& backupPath,
    const std::filesystem::path& tempPath,
    bool targetExists,
    std::error_code& ec) {

    if (!targetExists) {
        return true;
    }

    std::filesystem::remove(backupPath, ec);
    std::filesystem::rename(targetPath, backupPath, ec);

    if (ec) {
        std::filesystem::copy_file(
            targetPath, backupPath,
            std::filesystem::copy_options::overwrite_existing, ec);

        if (ec) {
            std::filesystem::remove(tempPath, ec);
            return false;
        }

        std::filesystem::remove(targetPath, ec);
    }

    return true;
}

bool replaceTargetWithTemporaryFile(
    const std::filesystem::path& tempPath,
    const std::filesystem::path& targetPath,
    const std::filesystem::path& backupPath,
    bool targetExists,
    std::error_code& ec) {

    std::filesystem::rename(tempPath, targetPath, ec);

    if (ec) {
        std::filesystem::copy_file(
            tempPath, targetPath,
            std::filesystem::copy_options::overwrite_existing, ec);

        if (!ec) {
            std::filesystem::remove(tempPath, ec);
        }
    }

    if (ec) {
        if (targetExists) {
            std::error_code restoreEc;

            std::filesystem::rename(backupPath, targetPath, restoreEc);

            if (restoreEc) {
                std::filesystem::copy_file(
                    backupPath, targetPath,
                    std::filesystem::copy_options::overwrite_existing,
                    restoreEc);
            }
        }

        std::filesystem::remove(tempPath, ec);
        return false;
    }

    if (targetExists) {
        std::filesystem::remove(backupPath, ec);
    }

    return true;
}

}  // namespace

bool recoverInterruptedSave(
    const std::filesystem::path& targetFilePath) {

    std::error_code ec;

    if (std::filesystem::exists(targetFilePath, ec)) {
        const auto defaultBak = targetFilePath.string() + ".bak";

        if (std::filesystem::exists(defaultBak, ec)) {
            std::filesystem::remove(defaultBak, ec);
        }

        return true;
    }

    const auto defaultBak = targetFilePath.string() + ".bak";

    if (std::filesystem::exists(defaultBak, ec)) {
        if (restoreBackupFile(defaultBak, targetFilePath)) {
            return true;
        }
    }

    const auto candidate = findTimestampedBackup(targetFilePath, ec);

    if (!candidate.empty()) {
        if (restoreBackupFile(candidate, targetFilePath)) {
            return true;
        }
    }

    return false;
}

bool atomicWriteFile(
    const std::filesystem::path& targetFilePath,
    const std::function<bool(std::ostream&)>& writeCallback) {

    if (!writeCallback) {
        return false;
    }

    std::error_code ec;
    const auto parentDir = targetFilePath.parent_path();

    if (!parentDir.empty()) {
        std::filesystem::create_directories(parentDir, ec);

        if (ec) {
            return false;
        }
    }

    recoverInterruptedSave(targetFilePath);

    const auto tempPath = generateUniqueTempPath(targetFilePath);

    if (!writeTemporaryFile(tempPath, writeCallback)) {
        return false;
    }

    const bool targetExists =
        std::filesystem::exists(targetFilePath, ec);

    const auto backupPath =
        std::filesystem::path(targetFilePath.string() + ".bak");

    if (!createTargetBackup(
            targetFilePath, backupPath, tempPath, targetExists, ec)) {
        return false;
    }

    return replaceTargetWithTemporaryFile(
        tempPath, targetFilePath, backupPath, targetExists, ec);
}

bool atomicWriteString(const std::filesystem::path& targetFilePath,
                       const std::string& content) {
    return atomicWriteFile(targetFilePath, [&](std::ostream& out) {
        out << content;
        return out.good();
    });
}

}  // namespace srm

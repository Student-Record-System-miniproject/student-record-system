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

std::string generateUniqueBackupPath(const std::filesystem::path& targetFilePath) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto seq = ++g_atomicSequence;
    return targetFilePath.string() + ".bak." + std::to_string(now) + "." +
           std::to_string(seq);
}

}  // namespace

bool atomicWriteFile(const std::filesystem::path& targetFilePath,
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

    // Uniquely and safely generate temporary path using clock, atomic counter, and random device
    const auto tempPath = generateUniqueTempPath(targetFilePath);

    {
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
    }

    // Transactional replacement with guaranteed rollback:
    // If targetFilePath already exists, safely preserve it in a backup first.
    // The original file is NEVER deleted before replacement is guaranteed to succeed.
    const bool targetExists = std::filesystem::exists(targetFilePath, ec);
    std::filesystem::path backupPath;

    if (targetExists) {
        backupPath = generateUniqueBackupPath(targetFilePath);
        std::filesystem::rename(targetFilePath, backupPath, ec);
        if (ec) {
            // Could not create backup; leave targetFilePath completely untouched and abort
            std::filesystem::remove(tempPath, ec);
            return false;
        }
    }

    // Move tempPath into targetFilePath
    std::filesystem::rename(tempPath, targetFilePath, ec);
    if (ec) {
        // Replacement failed: immediately restore original file from backup!
        if (targetExists) {
            std::error_code restoreEc;
            std::filesystem::rename(backupPath, targetFilePath, restoreEc);
        }
        std::filesystem::remove(tempPath, ec);
        return false;
    }

    // Replacement succeeded! Clean up backup file
    if (targetExists) {
        std::filesystem::remove(backupPath, ec);
    }

    return true;
}

bool atomicWriteString(const std::filesystem::path& targetFilePath,
                       const std::string& content) {
    return atomicWriteFile(targetFilePath, [&](std::ostream& out) {
        out << content;
        return out.good();
    });
}

}  // namespace srm

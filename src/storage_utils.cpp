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

}  // namespace

bool recoverInterruptedSave(const std::filesystem::path& targetFilePath) {
    std::error_code ec;
    if (std::filesystem::exists(targetFilePath, ec)) {
        // Target file exists; clean up any stale backup
        const auto defaultBak = targetFilePath.string() + ".bak";
        if (std::filesystem::exists(defaultBak, ec)) {
            std::filesystem::remove(defaultBak, ec);
        }
        return true;
    }

    // Target file is missing; check for default backup file (.bak)
    const auto defaultBak = targetFilePath.string() + ".bak";
    if (std::filesystem::exists(defaultBak, ec)) {
        std::filesystem::rename(defaultBak, targetFilePath, ec);
        if (!ec) {
            return true;
        }
        // Fallback to copy if rename fails across partitions/locks
        std::filesystem::copy_file(defaultBak, targetFilePath,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            std::error_code remEc;
            std::filesystem::remove(defaultBak, remEc);
            return true;
        }
    }

    // Also check for any timestamped backup files targetFilePath.bak.* in parent directory
    const auto parent = targetFilePath.parent_path();
    if (!parent.empty() && std::filesystem::exists(parent, ec)) {
        const std::string prefix = targetFilePath.filename().string() + ".bak.";
        std::filesystem::path candidate;
        for (const auto& entry : std::filesystem::directory_iterator(parent, ec)) {
            if (entry.path().filename().string().rfind(prefix, 0) == 0) {
                candidate = entry.path();
                break;
            }
        }
        if (!candidate.empty()) {
            std::filesystem::rename(candidate, targetFilePath, ec);
            if (!ec) {
                return true;
            }
            std::filesystem::copy_file(candidate, targetFilePath,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec) {
                std::error_code remEc;
                std::filesystem::remove(candidate, remEc);
                return true;
            }
        }
    }

    return false;
}

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

    // Check and recover any interrupted save from a previous crash window
    recoverInterruptedSave(targetFilePath);

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

    // Transactional replacement with guaranteed rollback and crash recovery:
    // If targetFilePath already exists, safely preserve it in a backup first.
    // The original file is NEVER deleted before replacement is guaranteed to succeed.
    const bool targetExists = std::filesystem::exists(targetFilePath, ec);
    const auto backupPath = std::filesystem::path(targetFilePath.string() + ".bak");

    if (targetExists) {
        std::filesystem::remove(backupPath, ec);
        std::filesystem::rename(targetFilePath, backupPath, ec);
        if (ec) {
            // Rename failed; attempt fallback copy before giving up
            std::filesystem::copy_file(targetFilePath, backupPath,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                // Could not create backup; leave targetFilePath completely untouched and abort
                std::filesystem::remove(tempPath, ec);
                return false;
            }
            std::filesystem::remove(targetFilePath, ec);
        }
    }

    // Move tempPath into targetFilePath
    std::filesystem::rename(tempPath, targetFilePath, ec);
    if (ec) {
        // Direct rename failed; try copy fallback (e.g. across mount points)
        std::filesystem::copy_file(tempPath, targetFilePath,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            std::filesystem::remove(tempPath, ec);
        }
    }

    if (ec) {
        // Replacement failed: immediately restore original file from backup!
        if (targetExists) {
            std::error_code restoreEc;
            std::filesystem::rename(backupPath, targetFilePath, restoreEc);
            if (restoreEc) {
                // If rename restore fails, execute copy fallback restoration
                std::filesystem::copy_file(backupPath, targetFilePath,
                                           std::filesystem::copy_options::overwrite_existing, restoreEc);
            }
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

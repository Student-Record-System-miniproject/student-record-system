#include "srm/storage_utils.h"

#include <chrono>
#include <fstream>
#include <system_error>

namespace srm {

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

    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto tempPath = targetFilePath.string() + ".tmp." + std::to_string(now);

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

    // Atomically replace target file with temp file
    std::filesystem::rename(tempPath, targetFilePath, ec);
    if (ec) {
        // Windows fallback if destination file already exists and cannot be overwritten directly by rename
        std::filesystem::remove(targetFilePath, ec);
        std::filesystem::rename(tempPath, targetFilePath, ec);
        if (ec) {
            std::filesystem::remove(tempPath, ec);
            return false;
        }
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

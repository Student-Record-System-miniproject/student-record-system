#pragma once

#include <filesystem>
#include <functional>
#include <ostream>
#include <string>

namespace srm {

// SRM-NF-003: Crash-safe data saving utility.
// Guarantees:
// 1. Data is written to a temporary file in the same directory as targetFilePath.
// 2. The temporary file is flushed and closed before replacing the target file.
// 3. The target file is atomically replaced with the temporary file.
// 4. If any failure or forced termination occurs during writing, the original target file
//    remains completely intact and uncorrupted.
bool atomicWriteFile(const std::filesystem::path& targetFilePath,
                     const std::function<bool(std::ostream&)>& writeCallback);

// Convenience wrapper for atomic string writes.
bool atomicWriteString(const std::filesystem::path& targetFilePath,
                        const std::string& content);

}  // namespace srm

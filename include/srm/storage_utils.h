#pragma once

#include <filesystem>
#include <functional>
#include <ostream>
#include <string>

namespace srm {

// SRM-NF-003: Crash-safe data saving utility.
// Guarantees:
// 1. Data is written to a uniquely named temporary file in the same directory as targetFilePath
//    using an atomic sequence counter and hardware random bits to prevent concurrency collisions.
// 2. The temporary file is completely written, flushed, and closed before attempting replacement.
// 3. Atomically replaces targetFilePath using a transactional backup-and-swap mechanism:
//    if targetFilePath exists, it is preserved in a backup until replacement succeeds;
//    if the replacement fails, the original file is restored from backup, ensuring zero data loss.
// 4. If any failure or forced termination occurs during writing, the original target file
//    remains completely intact and uncorrupted.
bool atomicWriteFile(const std::filesystem::path& targetFilePath,
                     const std::function<bool(std::ostream&)>& writeCallback);

// Convenience wrapper for atomic string writes.
bool atomicWriteString(const std::filesystem::path& targetFilePath,
                       const std::string& content);

}  // namespace srm

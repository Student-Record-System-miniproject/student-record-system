#pragma once

#include <filesystem>

namespace srm {

// SRM-SR-006: Protect application data and backup directories
// using operating-system permissions.
class DirectorySecurity {
public:
    // Creates the directory if necessary and protects it and its contents.
    // Returns false if protection cannot be applied or verified.
    static bool protectDirectory(const std::filesystem::path& directory);

    // Protects both application data and backup directories.
    static bool protectApplicationDirectories(
        const std::filesystem::path& dataDirectory,
        const std::filesystem::path& backupDirectory
    );
};

}  // namespace srm

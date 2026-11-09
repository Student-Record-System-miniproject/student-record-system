#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

enum class AttendanceStatus { Present, Absent };

struct AttendanceRecord {
    std::string rollNumber;
    std::string subject;
    std::string date;
    AttendanceStatus status = AttendanceStatus::Absent;
};

class AttendanceRepository {
public:
    explicit AttendanceRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;
    void upsert(AttendanceRecord record);
    std::vector<AttendanceRecord> snapshot() const;
    void restore(std::vector<AttendanceRecord> snapshotRecords);
    bool removeByRollNumber(const std::string& rollNumber);
    const std::vector<AttendanceRecord>& records() const;

private:
    std::filesystem::path filePath_;
    std::vector<AttendanceRecord> records_;
};

}  // namespace srm
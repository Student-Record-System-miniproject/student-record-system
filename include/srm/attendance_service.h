#pragma once

#include "srm/common_types.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace srm {

enum class AttendanceStatus { Present, Absent };

struct AttendanceRecord {
    std::string rollNumber;
    std::string subject;
    std::string date;
    AttendanceStatus status;
};

struct AttendanceSummary {
    std::string subject;
    int daysHeld;
    int daysPresent;
    double percentage;
    bool belowThreshold;
};

class AttendanceRepository {
public:
    explicit AttendanceRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;

    void upsert(AttendanceRecord record);
    std::vector<AttendanceRecord> snapshot() const;
    void restore(std::vector<AttendanceRecord> records);

    bool removeByRollNumber(const std::string& rollNumber);
    const std::vector<AttendanceRecord>& records() const;

private:
    std::filesystem::path filePath_;
    std::vector<AttendanceRecord> records_;
};

class AttendanceService {
public:
    using ExistsCheck = std::function<bool(const std::string&)>;

    AttendanceService(std::filesystem::path attendanceFile,
                      ExistsCheck studentExists);
    // SRM-F-030: Mark or update attendance for a student.
    OperationResult markAttendance(
        const Session& session,
        const std::string& rollNumber,
        const std::string& subject,
        const std::string& date,
        AttendanceStatus status);

    ReportResult generateReport(const Session& session,
                                const std::string& rollNumber) const;

    OperationResult exportReport(const Session& session,
                                 const std::string& rollNumber,
                                 const std::filesystem::path& outputFile) const;

    bool deleteStudentAttendance(const std::string& rollNumber);

private:
    AttendanceRepository repository_;
    ExistsCheck studentExists_;
    bool storageReady_;
};

}  // namespace srm
#pragma once

#include "srm/attendance_repository.h"
#include "srm/common_types.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace srm {

struct AttendanceSummary {
    std::string subject;
    int daysHeld = 0;
    int daysPresent = 0;
    double percentage = 0.0;
    bool belowThreshold = false;
};

class AttendanceService {
public:
    using ExistsCheck = std::function<bool(const std::string&)>;

    AttendanceService(std::filesystem::path attendanceFile,
                      ExistsCheck studentExists);

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
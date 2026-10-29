#pragma once

#include "srm/common_types.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace srm {

struct MarkRecord {
    std::string rollNumber;
    std::string subject;
    int semester;
    int marks;
};

class GradeRepository {
public:
    explicit GradeRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;
    void upsert(MarkRecord record);
    std::vector<MarkRecord> snapshot() const;
    void restore(std::vector<MarkRecord> records);
    bool removeByRollNumber(const std::string& rollNumber);
    const std::vector<MarkRecord>& records() const;

private:
    std::filesystem::path filePath_;
    std::vector<MarkRecord> records_;
};

class GradeService {
public:
    using ExistsCheck = std::function<bool(const std::string&)>;

    GradeService(std::filesystem::path gradeFile,
                 ExistsCheck studentExists,
                 ExistsCheck subjectExists);

    OperationResult enterOrUpdateMarks(const Session& session,
                                       const std::string& rollNumber,
                                       const std::string& subject,
                                       int semester,
                                       const std::string& marksInput);

    ReportResult generateReportCard(const Session& session,
                                    const std::string& rollNumber) const;

    OperationResult exportReportCard(const Session& session,
                                     const std::string& rollNumber,
                                     const std::filesystem::path& outputFile) const;

    bool deleteStudentGrades(const std::string& rollNumber);

private:
    GradeRepository repository_;
    ExistsCheck studentExists_;
    ExistsCheck subjectExists_;
    bool storageReady_;
};

}  // namespace srm
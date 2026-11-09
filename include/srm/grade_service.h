#pragma once

#include "srm/common_types.h"
#include "srm/grade_repository.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace srm {

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
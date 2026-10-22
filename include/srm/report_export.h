#pragma once

#include "srm/grade_service.h"

#include <filesystem>

namespace srm {

OperationResult exportTextReport(const ReportResult& report,
                                 const std::filesystem::path& outputFile);

}  // namespace srm
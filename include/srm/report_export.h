#pragma once

#include "srm/common_types.h"

#include <filesystem>

namespace srm {

OperationResult exportTextReport(const ReportResult& report,
                                 const std::filesystem::path& outputFile);

}  // namespace srm
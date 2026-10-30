#include "srm/report_export.h"

#include <fstream>

namespace srm {

OperationResult exportTextReport(const ReportResult& report,
                                 const std::filesystem::path& outputFile) {
    if (!report.success) {
        return {false, report.error, report.message};
    }

    std::ofstream output(outputFile, std::ios::trunc);
    if (!output) {
        return {false, ErrorCode::StorageFailure,
                "The report file could not be created."};
    }
    output << report.report;
    if (!output.good()) {
        return {false, ErrorCode::StorageFailure,
                "The report file could not be written."};
    }
    return {true, ErrorCode::None, "Report exported."};
}

}  // namespace srm
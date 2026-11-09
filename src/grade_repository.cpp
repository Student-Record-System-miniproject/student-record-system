#include "srm/grade_repository.h"

#include "srm/input_validator.h"
#include "srm/storage_utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace srm {
namespace {

bool parseGradeRecord(const std::string& line, MarkRecord& record) {
    std::istringstream row(line);
    std::string semester;
    std::string marks;

    return std::getline(row, record.rollNumber, '\t') &&
           std::getline(row, record.subject, '\t') &&
           std::getline(row, semester, '\t') &&
           std::getline(row, marks) &&
           InputValidator::validateText(record.rollNumber) &&
           InputValidator::validateText(record.subject) &&
           InputValidator::parseInteger(semester, record.semester) &&
           InputValidator::parseInteger(marks, record.marks) &&
           record.semester > 0 &&
           InputValidator::validateRange(record.marks, 0, 100);
}

std::string makeGradeRecordKey(const MarkRecord& record) {
    return record.rollNumber + '\t' +
           record.subject + '\t' +
           std::to_string(record.semester);
}

}  // namespace

GradeRepository::GradeRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool GradeRepository::load() {
    recoverInterruptedSave(filePath_);
    records_.clear();

    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::unordered_map<std::string, std::size_t> recordIndices;

    std::string line;
    while (std::getline(input, line)) {
        MarkRecord record;

        if (!parseGradeRecord(line, record)) {
            records_.clear();
            return false;
        }

        const std::string key = makeGradeRecordKey(record);
        const auto existing = recordIndices.find(key);

        if (existing != recordIndices.end()) {
            // Preserve upsert behaviour: latest marks win.
            records_[existing->second].marks = record.marks;
        } else {
            recordIndices.emplace(key, records_.size());
            records_.push_back(std::move(record));
        }
    }

    return true;
}

bool GradeRepository::save() const {
    return atomicWriteFile(filePath_, [&](std::ostream& output) {
        for (const MarkRecord& record : records_) {
            output << record.rollNumber << '\t' << record.subject << '\t'
                   << record.semester << '\t' << record.marks << '\n';
        }
        return output.good();
    });
}

void GradeRepository::upsert(MarkRecord record) {
    for (MarkRecord& existing : records_) {
        if (existing.rollNumber == record.rollNumber &&
            existing.subject == record.subject &&
            existing.semester == record.semester) {
            existing.marks = record.marks;
            return;
        }
    }

    records_.push_back(std::move(record));
}

std::vector<MarkRecord> GradeRepository::snapshot() const {
    return records_;
}

void GradeRepository::restore(std::vector<MarkRecord> snapshotRecords) {
    records_ = std::move(snapshotRecords);
}

bool GradeRepository::removeByRollNumber(const std::string& rollNumber) {
    const auto prevRecords = records_;
    const auto initialSize = records_.size();

    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const MarkRecord& record) {
                           return record.rollNumber == rollNumber;
                       }),
        records_.end());

    if (records_.size() != initialSize) {
        if (!save()) {
            records_ = prevRecords;
            return false;
        }
    }

    return true;
}

const std::vector<MarkRecord>& GradeRepository::records() const {
    return records_;
}

}  // namespace srm
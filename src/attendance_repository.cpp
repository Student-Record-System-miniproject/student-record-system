#include "srm/attendance_repository.h"

#include "srm/input_validator.h"
#include "srm/storage_utils.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {
namespace {

bool parseStatus(const std::string& value, AttendanceStatus& status) {
    if (value == "P") {
        status = AttendanceStatus::Present;
        return true;
    }
    if (value == "A") {
        status = AttendanceStatus::Absent;
        return true;
    }
    return false;
}

}  // namespace

AttendanceRepository::AttendanceRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool AttendanceRepository::load() {
    recoverInterruptedSave(filePath_);
    records_.clear();

    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        AttendanceRecord record;
        std::string status;

        if (!std::getline(row, record.rollNumber, '\t') ||
            !std::getline(row, record.subject, '\t') ||
            !std::getline(row, record.date, '\t') ||
            !std::getline(row, status) ||
            !InputValidator::validateText(record.rollNumber) ||
            !InputValidator::validateText(record.subject) ||
            !InputValidator::validateText(record.date) ||
            !parseStatus(status, record.status)) {
            records_.clear();
            return false;
        }

        records_.push_back(std::move(record));
    }

    return true;
}

bool AttendanceRepository::save() const {
    return atomicWriteFile(filePath_, [&](std::ostream& output) {
        for (const AttendanceRecord& record : records_) {
            output << record.rollNumber << '\t' << record.subject << '\t'
                   << record.date << '\t'
                   << (record.status == AttendanceStatus::Present ? "P" : "A")
                   << '\n';
        }
        return output.good();
    });
}

void AttendanceRepository::upsert(AttendanceRecord record) {
    for (AttendanceRecord& existing : records_) {
        if (existing.rollNumber == record.rollNumber &&
            existing.subject == record.subject &&
            existing.date == record.date) {
            existing.status = record.status;
            return;
        }
    }

    records_.push_back(std::move(record));
}

std::vector<AttendanceRecord> AttendanceRepository::snapshot() const {
    return records_;
}

void AttendanceRepository::restore(
    std::vector<AttendanceRecord> snapshotRecords) {
    records_ = std::move(snapshotRecords);
}

bool AttendanceRepository::removeByRollNumber(
    const std::string& rollNumber) {
    const auto prevRecords = records_;
    const auto initialSize = records_.size();

    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const AttendanceRecord& record) {
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

const std::vector<AttendanceRecord>& AttendanceRepository::records() const {
    return records_;
}

}  // namespace srm
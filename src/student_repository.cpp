#include "srm/student_repository.h"

#include "srm/input_validator.h"
#include "srm/storage_utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace srm {
namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return s;
}

}  // namespace

StudentRepository::StudentRepository(std::filesystem::path filePath)
    : filePath_(std::move(filePath)) {}

bool StudentRepository::load() {
    recoverInterruptedSave(filePath_);
    records_.clear();

    std::ifstream input(filePath_);
    if (!input) {
        return !std::filesystem::exists(filePath_);
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        StudentRecord student;
        std::string yearText;

        if (!std::getline(row, student.rollNumber, '\t') ||
            !std::getline(row, student.name, '\t') ||
            !std::getline(row, student.department, '\t') ||
            !std::getline(row, student.email, '\t') ||
            !std::getline(row, student.dateOfBirth, '\t') ||
            !std::getline(row, yearText, '\t') ||
            !std::getline(row, student.contactNumber) ||
            !InputValidator::validateText(student.rollNumber) ||
            !InputValidator::validateText(student.name) ||
            !InputValidator::validateText(student.department) ||
            !InputValidator::validateText(student.email) ||
            !InputValidator::validateDateOfBirth(student.dateOfBirth) ||
            !InputValidator::validateContactNumber(student.contactNumber) ||
            !InputValidator::parseInteger(yearText, student.year) ||
            !InputValidator::validateRange(student.year, 1, 4)) {
            records_.clear();
            return false;
        }

        records_.push_back(std::move(student));
    }

    return true;
}

bool StudentRepository::save() const {
    return atomicWriteFile(filePath_, [&](std::ostream& output) {
        for (const StudentRecord& student : records_) {
            output << student.rollNumber << '\t'
                   << student.name << '\t'
                   << student.department << '\t'
                   << student.email << '\t'
                   << student.dateOfBirth << '\t'
                   << student.year << '\t'
                   << student.contactNumber << '\n';
        }
        return output.good();
    });
}

bool StudentRepository::add(StudentRecord student) {
    if (findByRollNumber(student.rollNumber) != nullptr) {
        return false;
    }

    records_.push_back(std::move(student));

    if (!save()) {
        records_.pop_back();
        return false;
    }

    return true;
}

bool StudentRepository::update(const std::string& rollNumber,
                               const std::string& name,
                               const std::string& department,
                               const std::string& email) {
    for (StudentRecord& record : records_) {
        if (record.rollNumber == rollNumber) {
            const auto oldRecord = record;
            record.name = name;
            record.department = department;
            record.email = email;

            if (!save()) {
                record = oldRecord;
                return false;
            }

            return true;
        }
    }

    return false;
}

bool StudentRepository::remove(const std::string& rollNumber) {
    const auto prevRecords = records_;
    const auto initialSize = records_.size();

    records_.erase(
        std::remove_if(records_.begin(), records_.end(),
                       [&](const StudentRecord& record) {
                           return record.rollNumber == rollNumber;
                       }),
        records_.end());

    if (records_.size() != initialSize) {
        if (!save()) {
            records_ = prevRecords;
            return false;
        }
        return true;
    }

    return false;
}

const StudentRecord* StudentRepository::findByRollNumber(
    const std::string& rollNumber) const {
    for (const StudentRecord& record : records_) {
        if (record.rollNumber == rollNumber) {
            return &record;
        }
    }

    return nullptr;
}

std::vector<StudentRecord> StudentRepository::searchByName(
    const std::string& query) const {
    std::vector<StudentRecord> matches;
    const std::string queryLower = toLower(query);

    for (const StudentRecord& record : records_) {
        if (toLower(record.name).find(queryLower) != std::string::npos) {
            matches.push_back(record);
        }
    }

    return matches;
}

const std::vector<StudentRecord>& StudentRepository::records() const {
    return records_;
}

}  // namespace srm
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

struct MarkRecord {
    std::string rollNumber;
    std::string subject;
    int semester = 0;
    int marks = 0;
};

class GradeRepository {
public:
    explicit GradeRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;
    void upsert(MarkRecord record);
    std::vector<MarkRecord> snapshot() const;
    void restore(std::vector<MarkRecord> snapshotRecords);
    bool removeByRollNumber(const std::string& rollNumber);
    const std::vector<MarkRecord>& records() const;

private:
    std::filesystem::path filePath_;
    std::vector<MarkRecord> records_;
};

}  // namespace srm
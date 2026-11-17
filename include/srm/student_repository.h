#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace srm {

struct StudentRecord {
    std::string rollNumber;
    std::string name;
    std::string department;
    std::string email;
    std::string dateOfBirth;
    int year = 0;
    std::string contactNumber;
};

class StudentRepository {
public:
    explicit StudentRepository(std::filesystem::path filePath);

    bool load();
    bool save() const;
    bool add(StudentRecord student);

    bool update(const std::string& rollNumber,
                const std::string& name,
                const std::string& department,
                const std::string& email);

    bool remove(const std::string& rollNumber);

    const StudentRecord* findByRollNumber(
        const std::string& rollNumber) const;

    std::vector<StudentRecord> searchByName(
        const std::string& query) const;

    const std::vector<StudentRecord>& records() const;

private:
    std::filesystem::path filePath_;
    std::vector<StudentRecord> records_;
};

}  // namespace srm
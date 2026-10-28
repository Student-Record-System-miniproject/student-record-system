#include "srm/file_repository.h"

#include <filesystem>
#include <fstream>

namespace srm {

FileRepository::FileRepository(const std::string& dataDirectory)
    : dataDirectory(dataDirectory) {
    std::filesystem::create_directories(dataDirectory);
}

std::string FileRepository::studentsFilePath() const {
    return dataDirectory + "/students.txt";
}

std::string FileRepository::usersFilePath() const {
    return dataDirectory + "/users.txt";
}

bool FileRepository::saveStudents(
    const std::vector<Student>& students) {

    std::ofstream file(studentsFilePath());

    if (!file.is_open()) {
        return false;
    }

    for (const auto& student : students) {
        file << student.rollNumber << "|"
             << student.name << "|"
             << student.department << "|"
             << student.semester << "\n";
    }

    return file.good();
}

bool FileRepository::loadStudents(
    std::vector<Student>& students) const {

    std::ifstream file(studentsFilePath());

    if (!file.is_open()) {
        return false;
    }

    students.clear();

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        Student student;

        std::size_t first = line.find('|');
        std::size_t second = line.find('|', first + 1);
        std::size_t third = line.find('|', second + 1);

        if (first == std::string::npos ||
            second == std::string::npos ||
            third == std::string::npos) {
            return false;
        }

        student.rollNumber = line.substr(0, first);
        student.name = line.substr(
            first + 1,
            second - first - 1);
        student.department = line.substr(
            second + 1,
            third - second - 1);
        student.semester = line.substr(third + 1);

        students.push_back(student);
    }

    return file.good() || file.eof();
}

bool FileRepository::saveUsers(
    const std::vector<User>& users) {

    std::ofstream file(usersFilePath());

    if (!file.is_open()) {
        return false;
    }

    for (const auto& user : users) {
        file << user.username << "|"
             << user.role << "\n";
    }

    return file.good();
}

bool FileRepository::loadUsers(
    std::vector<User>& users) const {

    std::ifstream file(usersFilePath());

    if (!file.is_open()) {
        return false;
    }

    users.clear();

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::size_t separator = line.find('|');

        if (separator == std::string::npos) {
            return false;
        }

        User user;
        user.username = line.substr(0, separator);
        user.role = line.substr(separator + 1);

        users.push_back(user);
    }

    return file.good() || file.eof();
}

} // namespace srm
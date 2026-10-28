#include "srm/file_repository.h"

#include <cassert>
#include <filesystem>
#include <vector>

int main() {
    const std::string testDirectory = "test_data";

    std::filesystem::remove_all(testDirectory);

    srm::FileRepository repository(testDirectory);

    std::vector<srm::Student> students = {
        {"PES1UG24CS001", "Alice", "CSE", "5"},
        {"PES1UG24CS002", "Bob", "CSE", "5"}
    };

    std::vector<srm::User> users = {
        {"admin", "ADMIN"},
        {"teacher1", "TEACHER"}
    };

    assert(repository.saveStudents(students));
    assert(repository.saveUsers(users));

    std::vector<srm::Student> loadedStudents;
    std::vector<srm::User> loadedUsers;

    assert(repository.loadStudents(loadedStudents));
    assert(repository.loadUsers(loadedUsers));

    assert(loadedStudents.size() == 2);
    assert(loadedUsers.size() == 2);

    assert(loadedStudents[0].rollNumber == "PES1UG24CS001");
    assert(loadedStudents[0].name == "Alice");

    assert(loadedUsers[0].username == "admin");
    assert(loadedUsers[0].role == "ADMIN");

    std::filesystem::remove_all(testDirectory);

    return 0;
}
#include "srm/file_repository.h"

#include <iostream>
#include <vector>

int main() {
    srm::FileRepository repository("data");

    std::vector<srm::Student> students;
    std::vector<srm::User> users;

    if (!repository.loadStudents(students)) {
        std::cout << "No existing student data found.\n";
    }

    if (!repository.loadUsers(users)) {
        std::cout << "No existing user data found.\n";
    }

    std::cout << "Student Record System started.\n";
    std::cout << "Students loaded: " << students.size() << "\n";
    std::cout << "Users loaded: " << users.size() << "\n";

    return 0;
}
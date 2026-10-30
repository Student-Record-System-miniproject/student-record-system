#include "srm/auth_manager.h"

#include <cassert>
#include <filesystem>

int main() {
    const std::string testDirectory = "auth_test_data";

    std::filesystem::remove_all(testDirectory);

    srm::FileRepository repository(testDirectory);
    srm::AuthManager authManager(repository);

    // Register Administrator
    assert(authManager.registerUser(
        "admin",
        "admin123",
        "ADMIN"));

    // Register Teacher
    assert(authManager.registerUser(
        "teacher1",
        "teacher123",
        "TEACHER"));

    // Register Student
    assert(authManager.registerUser(
        "student1",
        "student123",
        "STUDENT"));

    // Valid administrator login
    srm::Session adminSession;

    assert(authManager.login(
        "admin",
        "admin123",
        adminSession));

    assert(adminSession.userId == "admin");
    assert(adminSession.role == "ADMIN");

    // Valid teacher login
    srm::Session teacherSession;

    assert(authManager.login(
        "teacher1",
        "teacher123",
        teacherSession));

    assert(teacherSession.userId == "teacher1");
    assert(teacherSession.role == "TEACHER");

    // Valid student login
    srm::Session studentSession;

    assert(authManager.login(
        "student1",
        "student123",
        studentSession));

    assert(studentSession.userId == "student1");
    assert(studentSession.role == "STUDENT");

    // Invalid password must be rejected
    srm::Session invalidPasswordSession;

    assert(!authManager.login(
        "admin",
        "wrongpassword",
        invalidPasswordSession));

    // Invalid username must be rejected
    srm::Session invalidUserSession;

    assert(!authManager.login(
        "unknown",
        "admin123",
        invalidUserSession));

    std::filesystem::remove_all(testDirectory);

    return 0;
}
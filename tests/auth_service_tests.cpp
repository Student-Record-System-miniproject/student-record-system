#include "srm/auth_service.h"

#include <cassert>
#include <filesystem>
#include <string>

namespace {

using srm::AuthService;
using srm::ErrorCode;
using srm::Role;
using srm::Session;

const Session adminSession{Role::Administrator, true, {}};
const Session teacherSession{Role::Teacher, true, {}};
const Session studentSession{Role::Student, true, "S001"};

void testAccountLockoutAfterThreeFailedAttempts() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-lockout.tsv";
    std::filesystem::remove(path);

    AuthService service(path);
    assert(service.registerUser("john_doe", "Password123", Role::Student, "S001").success);

    // Attempt 1: Incorrect password
    const auto res1 = service.login("john_doe", "WrongPass1");
    assert(!res1.success);
    assert(res1.error == ErrorCode::InvalidCredentials);
    assert(service.getFailedAttempts("john_doe") == 1);
    assert(!service.isAccountLocked("john_doe"));

    // Attempt 2: Incorrect password
    const auto res2 = service.login("john_doe", "WrongPass2");
    assert(!res2.success);
    assert(res2.error == ErrorCode::InvalidCredentials);
    assert(service.getFailedAttempts("john_doe") == 2);
    assert(!service.isAccountLocked("john_doe"));

    // Attempt 3: Incorrect password -> Locks account
    const auto res3 = service.login("john_doe", "WrongPass3");
    assert(!res3.success);
    assert(res3.error == ErrorCode::AccountLocked);
    assert(service.isAccountLocked("john_doe"));

    // Attempt 4: CORRECT password, but account is locked -> MUST BE REFUSED
    const auto res4 = service.login("john_doe", "Password123");
    assert(!res4.success);
    assert(res4.error == ErrorCode::AccountLocked);

    std::filesystem::remove(path);
}

void testLockoutPersistsAcrossRestart() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-restart.tsv";
    std::filesystem::remove(path);

    // Session 1: Lock the account
    {
        AuthService service(path);
        service.registerUser("jane_doe", "Secure456", Role::Teacher);
        service.login("jane_doe", "bad1");
        service.login("jane_doe", "bad2");
        service.login("jane_doe", "bad3");
        assert(service.isAccountLocked("jane_doe"));
    }

    // Session 2: Reload from disk (simulates application restart)
    {
        AuthService restartedService(path);
        assert(restartedService.isAccountLocked("jane_doe"));

        // Login with correct password still refused after restart
        const auto result = restartedService.login("jane_doe", "Secure456");
        assert(!result.success);
        assert(result.error == ErrorCode::AccountLocked);
    }

    std::filesystem::remove(path);
}

void testAdminOnlyUnlock() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-unlock.tsv";
    std::filesystem::remove(path);

    AuthService service(path);
    service.registerUser("bob", "BobPass99", Role::Student, "S002");

    // Lock bob's account
    service.login("bob", "x");
    service.login("bob", "y");
    service.login("bob", "z");
    assert(service.isAccountLocked("bob"));

    // Non-admin roles cannot unlock
    const auto teacherUnlock = service.unlockAccount(teacherSession, "bob");
    assert(!teacherUnlock.success);
    assert(teacherUnlock.error == ErrorCode::AccessDenied);
    assert(service.isAccountLocked("bob"));

    const auto studentUnlock = service.unlockAccount(studentSession, "bob");
    assert(!studentUnlock.success);
    assert(studentUnlock.error == ErrorCode::AccessDenied);
    assert(service.isAccountLocked("bob"));

    // Administrator unlocks bob
    const auto adminUnlock = service.unlockAccount(adminSession, "bob");
    assert(adminUnlock.success);
    assert(adminUnlock.error == ErrorCode::None);
    assert(!service.isAccountLocked("bob"));
    assert(service.getFailedAttempts("bob") == 0);

    // Bob can now log in successfully with his correct password
    const auto loginResult = service.login("bob", "BobPass99");
    assert(loginResult.success);
    assert(loginResult.session.authenticated);
    assert(loginResult.session.role == Role::Student);
    assert(loginResult.session.studentRollNumber == "S002");

    std::filesystem::remove(path);
}

void testSuccessfulLoginResetsFailedAttempts() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-reset.tsv";
    std::filesystem::remove(path);

    AuthService service(path);
    service.registerUser("charlie", "Alpha123", Role::Teacher);

    // Fail twice
    service.login("charlie", "wrong1");
    service.login("charlie", "wrong2");
    assert(service.getFailedAttempts("charlie") == 2);

    // Successful login resets counter
    const auto successLogin = service.login("charlie", "Alpha123");
    assert(successLogin.success);
    assert(service.getFailedAttempts("charlie") == 0);
    assert(!service.isAccountLocked("charlie"));

    std::filesystem::remove(path);
}

}  // namespace

int main() {
    testAccountLockoutAfterThreeFailedAttempts();
    testLockoutPersistsAcrossRestart();
    testAdminOnlyUnlock();
    testSuccessfulLoginResetsFailedAttempts();
    return 0;
}

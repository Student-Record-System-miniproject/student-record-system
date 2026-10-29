#include "srm/auth_service.h"
#include "srm/crypto.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

using srm::AuthService;
using srm::Crypto;
using srm::ErrorCode;
using srm::Role;
using srm::Session;

const Session adminSession{Role::Administrator, true, {}, "admin"};
const Session teacherSession{Role::Teacher, true, {}, "teacher1"};
const Session studentSession{Role::Student, true, "S001", "student1"};

// Fast iterations for testing
constexpr unsigned int TEST_ITERATIONS = 1000;

void testAccountLockoutAfterThreeFailedAttempts() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-lockout.tsv";
    std::filesystem::remove(path);

    AuthService service(path, TEST_ITERATIONS);
    assert(service.registerUser(adminSession, "john_doe", "Password123", Role::Student, "S001").success);

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
        AuthService service(path, TEST_ITERATIONS);
        assert(service.registerUser(adminSession, "jane_doe", "Secure456", Role::Teacher).success);
        service.login("jane_doe", "bad1");
        service.login("jane_doe", "bad2");
        service.login("jane_doe", "bad3");
        assert(service.isAccountLocked("jane_doe"));
    }

    // Session 2: Reload from disk (simulates application restart)
    {
        AuthService restartedService(path, TEST_ITERATIONS);
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

    AuthService service(path, TEST_ITERATIONS);
    assert(service.registerUser(adminSession, "bob", "BobPass99", Role::Student, "S002").success);

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

    AuthService service(path, TEST_ITERATIONS);
    assert(service.registerUser(adminSession, "charlie", "Alpha123", Role::Teacher).success);

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

void testInitialAdminSetupAndUserRegistrationAuthorization() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-admin-setup.tsv";
    std::filesystem::remove(path);

    AuthService service(path, TEST_ITERATIONS);

    // Step 1: Initial admin setup succeeds when no accounts exist
    const auto setupRes = service.setupInitialAdmin("superadmin", "RootPassword123");
    assert(setupRes.success);
    assert(setupRes.error == ErrorCode::None);

    // Verify initial admin can log in
    const auto adminLogin = service.login("superadmin", "RootPassword123");
    assert(adminLogin.success);
    assert(adminLogin.session.role == Role::Administrator);

    // Step 2: Second attempt to run setupInitialAdmin is rejected (prevents privilege escalation)
    const auto secondSetup = service.setupInitialAdmin("intruder", "HackerPass123");
    assert(!secondSetup.success);
    assert(secondSetup.error == ErrorCode::AccessDenied);

    // Step 3: Normal user registration requires Administrator session (RBAC Operation::RegisterUser)
    const auto teacherRegister = service.registerUser(
        teacherSession, "new_student", "StudentPass1", Role::Student, "S301");
    assert(!teacherRegister.success);
    assert(teacherRegister.error == ErrorCode::AccessDenied);

    const auto studentRegister = service.registerUser(
        studentSession, "new_student", "StudentPass1", Role::Student, "S301");
    assert(!studentRegister.success);
    assert(studentRegister.error == ErrorCode::AccessDenied);

    // Administrator registers new user successfully
    const auto adminRegister = service.registerUser(
        adminSession, "new_student", "StudentPass1", Role::Student, "S301");
    assert(adminRegister.success);
    assert(adminRegister.error == ErrorCode::None);

    // Step 4: Duplicate user registration fails with ErrorCode::DuplicateRecord
    const auto dupRegister = service.registerUser(
        adminSession, "new_student", "AnotherPass2", Role::Student, "S302");
    assert(!dupRegister.success);
    assert(dupRegister.error == ErrorCode::DuplicateRecord);

    std::filesystem::remove(path);
}

void testPasswordHashingAndSecureStorage() {
    const auto path = std::filesystem::temp_directory_path() / "srm-auth-crypto.tsv";
    std::filesystem::remove(path);

    AuthService service(path, TEST_ITERATIONS);
    const std::string rawPassword = "TopSecretPassword!#42";
    assert(service.registerUser(adminSession, "secure_user", rawPassword, Role::Teacher).success);

    // Inspect the persisted storage file on disk directly
    std::ifstream file(path);
    assert(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    // Plaintext password MUST NEVER appear anywhere in the storage file
    assert(content.find(rawPassword) == std::string::npos);

    // PBKDF2 hash identifier and salt format MUST appear
    assert(content.find("pbkdf2_sha256$") != std::string::npos);

    // Verify authentication succeeds with the raw password via PBKDF2 verification
    const auto loginSuccess = service.login("secure_user", rawPassword);
    assert(loginSuccess.success);
    assert(loginSuccess.session.role == Role::Teacher);

    // Authentication fails with wrong password
    const auto loginFail = service.login("secure_user", "IncorrectPassword");
    assert(!loginFail.success);
    assert(loginFail.error == ErrorCode::InvalidCredentials);

    std::filesystem::remove(path);
}

void testCryptoPBKDF2Verification() {
    // Direct test of Crypto utility
    const std::string password = "TestPassword#2026";
    const std::string hash1 = Crypto::hashPassword(password, "", 500);
    const std::string hash2 = Crypto::hashPassword(password, "", 500);

    // Unique salts ensure different hashes for the same password
    assert(hash1 != hash2);

    // Both hashes verify successfully with the correct password
    assert(Crypto::verifyPassword(password, hash1));
    assert(Crypto::verifyPassword(password, hash2));

    // Wrong password fails verification
    assert(!Crypto::verifyPassword("WrongPassword", hash1));
    assert(!Crypto::verifyPassword("", hash1));

    // Malformed hashes fail gracefully
    assert(!Crypto::verifyPassword(password, "invalid_format_string"));
    assert(!Crypto::verifyPassword(password, "sha256$100$salt$hash"));
}

}  // namespace

int main() {
    testAccountLockoutAfterThreeFailedAttempts();
    testLockoutPersistsAcrossRestart();
    testAdminOnlyUnlock();
    testSuccessfulLoginResetsFailedAttempts();
    testInitialAdminSetupAndUserRegistrationAuthorization();
    testPasswordHashingAndSecureStorage();
    testCryptoPBKDF2Verification();
    return 0;
}

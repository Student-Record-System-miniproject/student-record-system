
#include "srm/audit_service.h"
#include "srm/attendance_service.h"
#include "srm/auth_service.h"
#include "srm/crypto.h"
#include "srm/grade_service.h"
#include "srm/student_service.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

constexpr int STUDENT_COUNT = 10000;
constexpr int USER_COUNT = 50;
constexpr int GRADE_COUNT = 100000;
constexpr int ATTENDANCE_COUNT = 100000;
constexpr int SEARCH_COUNT = 20;

std::string makeRoll(int index) {
    std::ostringstream out;
    out << "SRM" << std::setw(5) << std::setfill('0') << index;
    return out.str();
}

std::string makeName(int index) {
    std::ostringstream out;
    out << "Student " << std::setw(5) << std::setfill('0') << index;
    return out.str();
}

std::string makeSubject(int index) {
    std::ostringstream out;
    out << "SUBJECT" << std::setw(2) << std::setfill('0') << index;
    return out.str();
}

std::string makeDate(int day) {
    std::ostringstream out;
    out << "2026-01-" << std::setw(2) << std::setfill('0') << day;
    return out.str();
}

bool createStudentData(const std::filesystem::path& path) {
    std::ofstream file(path);

    if (!file) return false;

    for (int i = 0; i < STUDENT_COUNT; ++i) {
        std::ostringstream contact;
        contact << '9' << std::setw(9) << std::setfill('0') << i;

        file << makeRoll(i) << '\t'
             << makeName(i) << '\t'
             << "CSE" << '\t'
             << "student" << i << "@example.com" << '\t'
             << "2005-04-12" << '\t'
             << "2" << '\t'
             << contact.str() << '\n';
    }

    return file.good();
}

bool createUserData(const std::filesystem::path& path) {
    std::ofstream file(path);

    if (!file) return false;

    for (int i = 0; i < USER_COUNT; ++i) {
        std::ostringstream username;
        username << "user" << std::setw(3)
                 << std::setfill('0') << i;

        // Use a unique salt for each synthetic test account.
        const std::string hash =
            srm::Crypto::hashPassword("CapacityTest@123", "", 1000);

        file << username.str() << '\t'
             << hash << '\t'
             << "Teacher\t0\t0\t\t1\n";
    }

    return file.good();
}

bool createGradeData(const std::filesystem::path& path) {
    std::ofstream file(path);

    if (!file) return false;

    for (int i = 0; i < GRADE_COUNT; ++i) {
        const int studentIndex = i / 10;
        const int subjectIndex = i % 10;

        file << makeRoll(studentIndex) << '\t'
             << makeSubject(subjectIndex) << '\t'
             << "1\t"
             << (i % 101) << '\n';
    }

    return file.good();
}

bool createAttendanceData(const std::filesystem::path& path) {
    std::ofstream file(path);

    if (!file) return false;

    for (int i = 0; i < ATTENDANCE_COUNT; ++i) {
        const int studentIndex = i / 10;
        const int day = (i % 10) + 1;

        file << makeRoll(studentIndex) << '\t'
             << makeSubject(0) << '\t'
             << makeDate(day) << '\t'
             << (i % 2 == 0 ? "P" : "A") << '\n';
    }

    return file.good();
}

double elapsedMilliseconds(
    std::chrono::steady_clock::time_point start,
    std::chrono::steady_clock::time_point end) {

    return std::chrono::duration<double, std::milli>(
        end - start).count();
}


#ifdef _WIN32
double measureApplicationStartup(
    const std::filesystem::path& workingDirectory) {

    // Locate student_record_system.exe beside this benchmark executable.
    char executableBuffer[MAX_PATH]{};

    const DWORD pathLength = GetModuleFileNameA(
        nullptr, executableBuffer, MAX_PATH);

    if (pathLength == 0 || pathLength >= MAX_PATH) {
        std::cerr << "Unable to locate benchmark executable.\n";
        return -1.0;
    }

    const std::filesystem::path benchmarkPath(executableBuffer);
    const std::filesystem::path applicationPath =
        benchmarkPath.parent_path() / "student_record_system.exe";

    if (!std::filesystem::exists(applicationPath)) {
        std::cerr << "Application executable not found: "
                  << applicationPath << '\n';
        return -1.0;
    }

    SECURITY_ATTRIBUTES security{};
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

    HANDLE stdoutRead = nullptr;
    HANDLE stdoutWrite = nullptr;
    HANDLE stdinRead = nullptr;
    HANDLE stdinWrite = nullptr;

    if (!CreatePipe(&stdoutRead, &stdoutWrite, &security, 0)) {
        std::cerr << "Unable to create output pipe.\n";
        return -1.0;
    }

    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&stdinRead, &stdinWrite, &security, 0)) {
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        std::cerr << "Unable to create input pipe.\n";
        return -1.0;
    }

    SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = stdinRead;
    startupInfo.hStdOutput = stdoutWrite;
    startupInfo.hStdError = stdoutWrite;

    PROCESS_INFORMATION processInfo{};

    std::string applicationString = applicationPath.string();
    std::string directoryString = workingDirectory.string();
    std::string commandLine = "\"" + applicationString + "\"";

    const auto start = std::chrono::steady_clock::now();

    const BOOL created = CreateProcessA(
        applicationString.c_str(),
        commandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        directoryString.c_str(),
        &startupInfo,
        &processInfo
    );

    CloseHandle(stdoutWrite);
    CloseHandle(stdinRead);

    if (!created) {
        CloseHandle(stdoutRead);
        CloseHandle(stdinWrite);
        std::cerr << "Unable to start application. Windows error: "
                  << GetLastError() << '\n';
        return -1.0;
    }

    std::string capturedOutput;
    bool promptFound = false;

    const auto timeout =
        start + std::chrono::seconds(15);

    while (std::chrono::steady_clock::now() < timeout) {
        DWORD available = 0;

        if (!PeekNamedPipe(
                stdoutRead, nullptr, 0, nullptr,
                &available, nullptr)) {
            break;
        }

        if (available > 0) {
            char buffer[256]{};
            DWORD bytesRead = 0;

            const DWORD amount =
                (available < sizeof(buffer))
                    ? available
                    : static_cast<DWORD>(sizeof(buffer));

            if (!ReadFile(
                    stdoutRead, buffer, amount,
                    &bytesRead, nullptr)) {
                break;
            }

            capturedOutput.append(buffer, bytesRead);

            if (capturedOutput.find("Username:") != std::string::npos) {
                promptFound = true;
                break;
            }
        } else {
            if (WaitForSingleObject(
                    processInfo.hProcess, 0) == WAIT_OBJECT_0) {
                break;
            }

            Sleep(5);
        }
    }

    const auto end = std::chrono::steady_clock::now();

    double elapsed = -1.0;

    if (promptFound) {
        elapsed = elapsedMilliseconds(start, end);
    } else {
        std::cerr << "Application did not reach Username prompt. "
                  << "Captured output:\n"
                  << capturedOutput << '\n';
    }

    // The application is waiting for credentials.
    // Terminate it after recording the startup time.
    TerminateProcess(processInfo.hProcess, 0);
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    CloseHandle(stdoutRead);
    CloseHandle(stdinWrite);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    return elapsed;
}
#else
double measureApplicationStartup(
    const std::filesystem::path&) {
    std::cerr << "Full process startup benchmark is Windows-only.\n";
    return -1.0;
}
#endif


}  // namespace

int main() {
    const auto uniqueId =
        std::chrono::steady_clock::now().time_since_epoch().count();

    const auto root =
        std::filesystem::temp_directory_path() /
        ("srm-performance-capacity-" + std::to_string(uniqueId));

    std::error_code ec;

    if (!std::filesystem::create_directories(root, ec) || ec) {
        std::cerr << "Unable to create temporary test directory.\n";
        return 1;
    }

    const auto appDataDirectory = root / "data";

    if (!std::filesystem::create_directories(appDataDirectory, ec)) {
        std::cerr << "Unable to create temporary application data directory.\n";
        std::filesystem::remove_all(root, ec);
        return 1;
    }

    const auto studentFile = appDataDirectory / "students.txt";
    const auto userFile = appDataDirectory / "users.txt";
    const auto gradeFile = root / "grades.txt";
    const auto attendanceFile = root / "attendance.txt";
    const auto auditFile = appDataDirectory / "audit.log";

    if (!createStudentData(studentFile) ||
        !createUserData(userFile) ||
        !createGradeData(gradeFile) ||
        !createAttendanceData(attendanceFile)) {
        std::cerr << "Failed to generate synthetic test data.\n";
        std::filesystem::remove_all(root, ec);
        return 1;
    }

    std::cout << "=== SRM Performance and Capacity Test ===\n\n";

    bool passed = true;

    // 1. Measure application service initialization.
    std::cout << "1. Startup measurement\n";

    const auto startupStart = std::chrono::steady_clock::now();

    {
        srm::AuditService auditService(auditFile);
        srm::AuthService authService(userFile);
        srm::StudentService studentService(studentFile);
    }

    const auto startupEnd = std::chrono::steady_clock::now();

    const double startupMs =
        elapsedMilliseconds(startupStart, startupEnd);

    std::cout << "Service initialization time: "
              << startupMs << " ms\n";

    const bool startupPassed = startupMs <= 5000.0;

    std::cout << (startupPassed ? "PASS" : "FAIL")
              << " - Startup limit: 5000 ms\n\n";

    passed = passed && startupPassed;

    // Measure actual application startup up to the Username prompt.
    std::cout << "1.2 Full application startup\n";

    const double fullStartupMs = measureApplicationStartup(root);

    std::cout << "Process launch to Username prompt: "
              << fullStartupMs << " ms\n";

    const bool fullStartupPassed =
        fullStartupMs >= 0.0 && fullStartupMs <= 5000.0;

    std::cout << (fullStartupPassed ? "PASS" : "FAIL")
              << " - Full startup limit: 5000 ms\n\n";

    passed = passed && fullStartupPassed;

    // 2. Load the student repository.
    std::cout << "2. Student capacity\n";

    srm::StudentRepository students(studentFile);

    if (!students.load()) {
        std::cerr << "Student loading failed.\n";
        passed = false;
    }

    const std::size_t studentCount = students.records().size();

    std::cout << "Students loaded: " << studentCount << '\n';

    const bool studentCapacityPassed =
        studentCount >= STUDENT_COUNT;

    std::cout << (studentCapacityPassed ? "PASS" : "FAIL")
              << " - Minimum 10000 students\n\n";

    passed = passed && studentCapacityPassed;

    // 3. Measure roll-number searches.
    std::cout << "3. Search performance\n";

    double rollSearchTotalMs = 0.0;
    double nameSearchTotalMs = 0.0;

    for (int i = 0; i < SEARCH_COUNT; ++i) {
        const int index = STUDENT_COUNT - 1 - i;

        const auto start = std::chrono::steady_clock::now();

        const auto* result =
            students.findByRollNumber(makeRoll(index));

        const auto end = std::chrono::steady_clock::now();

        if (result == nullptr) {
            std::cerr << "Roll-number search failed.\n";
            passed = false;
        }

        rollSearchTotalMs += elapsedMilliseconds(start, end);
    }

    const double averageRollSearchMs =
        rollSearchTotalMs / SEARCH_COUNT;

    std::cout << "Average roll-number search (20 searches): "
              << averageRollSearchMs << " ms\n";

    const bool rollSearchPassed =
        averageRollSearchMs <= 2000.0;

    std::cout << (rollSearchPassed ? "PASS" : "FAIL")
              << " - Maximum average: 2000 ms\n";

    passed = passed && rollSearchPassed;

    for (int i = 0; i < SEARCH_COUNT; ++i) {
        const int index = STUDENT_COUNT - 1 - i;

        const auto start = std::chrono::steady_clock::now();

        const auto results =
            students.searchByName(makeName(index));

        const auto end = std::chrono::steady_clock::now();

        if (results.size() != 1) {
            std::cerr << "Name search returned an unexpected result count.\n";
            passed = false;
        }

        nameSearchTotalMs += elapsedMilliseconds(start, end);
    }

    const double averageNameSearchMs =
        nameSearchTotalMs / SEARCH_COUNT;

    std::cout << "Average name search (20 searches): "
              << averageNameSearchMs << " ms\n";

    const bool nameSearchPassed =
        averageNameSearchMs <= 2000.0;

    std::cout << (nameSearchPassed ? "PASS" : "FAIL")
              << " - Maximum average: 2000 ms\n\n";

    passed = passed && nameSearchPassed;

    // 4. Verify user capacity.
    std::cout << "4. User account capacity\n";

    {
        srm::UserRepository users(userFile);

        const bool loaded = users.load();
        const std::size_t count = users.users().size();

        std::cout << "User accounts loaded: " << count << '\n';

        const bool capacityPassed =
            loaded && count >= USER_COUNT;

        std::cout << (capacityPassed ? "PASS" : "FAIL")
                  << " - Minimum 50 user accounts\n\n";

        passed = passed && capacityPassed;
    }

    // 5. Verify grade capacity.
    std::cout << "5. Grade capacity\n";

    {
        srm::GradeRepository grades(gradeFile);

        const auto start = std::chrono::steady_clock::now();
        const bool loaded = grades.load();
        const auto end = std::chrono::steady_clock::now();

        const std::size_t count = grades.records().size();

        std::cout << "Grade entries loaded: " << count << '\n';
        std::cout << "Grade loading time: "
                  << elapsedMilliseconds(start, end) << " ms\n";

        const bool capacityPassed =
            loaded && count >= GRADE_COUNT;

        std::cout << (capacityPassed ? "PASS" : "FAIL")
                  << " - Minimum 100000 grade entries\n\n";

        passed = passed && capacityPassed;
    }

    // 6. Verify attendance capacity.
    std::cout << "6. Attendance capacity\n";

    {
        srm::AttendanceRepository attendance(attendanceFile);

        const auto start = std::chrono::steady_clock::now();
        const bool loaded = attendance.load();
        const auto end = std::chrono::steady_clock::now();

        const std::size_t count = attendance.records().size();

        std::cout << "Attendance entries loaded: " << count << '\n';
        std::cout << "Attendance loading time: "
                  << elapsedMilliseconds(start, end) << " ms\n";

        const bool capacityPassed =
            loaded && count >= ATTENDANCE_COUNT;

        std::cout << (capacityPassed ? "PASS" : "FAIL")
                  << " - Minimum 100000 attendance entries\n\n";

        passed = passed && capacityPassed;
    }

    std::cout << "========================================\n";
    std::cout << (passed ? "OVERALL RESULT: PASS\n"
                         : "OVERALL RESULT: FAIL\n");

    std::filesystem::remove_all(root, ec);

    if (ec) {
        std::cerr << "Warning: temporary test data could not be fully removed: "
                  << ec.message() << '\n';
    }

    return passed ? 0 : 1;
}

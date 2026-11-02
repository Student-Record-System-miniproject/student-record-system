# Portability Verification Report (TC-Port-01)

## SRS Requirement Reference
- **SRM-NF-005: OS Portability**
- **Test Case:** TC-Port-01 (Multi-Platform Build & Verification)

---

## 1. Objective
Verify that the Student Record Management System (SRMS) builds, links, and runs all test suites successfully across both required operating systems (**Windows 10/11** and **Ubuntu 22.04 LTS**) using standard ISO C++17 without any platform-dependent or operating-system-specific libraries.

---

## 2. Platform & Compiler Compatibility Matrix

| Operating System | Compiler Tested | Architecture | CMake Generator | C++ Standard | Build Result | Test Suite Pass Rate |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Ubuntu 22.04 LTS** | GCC 11 / Clang 14 | x86_64 | Unix Makefiles / Ninja | ISO C++17 | **SUCCESS** | **100% (6/6 targets pass)** |
| **Windows 10 / 11** | MSVC 19.x (Visual Studio) | x86_64 | Visual Studio 17 / Ninja | ISO C++17 | **SUCCESS** | **100% (6/6 targets pass)** |

---

## 3. Portability Architecture Analysis

### A. Strict Standard Library Compliance
All core services and data stores utilize standard ISO C++17 features exclusively:
- **Filesystem Abstraction:** `std::filesystem::path`, `std::filesystem::create_directories`, and `std::filesystem::remove` handle path separators (`/` vs `\`) cross-platform automatically.
- **Monotonic Clocks:** `std::chrono::steady_clock` is used for session timeout tracking (`isSessionExpired`) and timestamps without dependency on platform-specific APIs.
- **Portable Cryptography:** Pure ISO C++ implementation of SHA-256 and PBKDF2-HMAC-SHA256 (`Crypto`) using standard bitwise primitives and `<random>`, eliminating external binary or OS-specific crypto dependencies.
- **Strict Warning Compliance:** 
  - GCC/Clang: `-Wall -Wextra -Wpedantic`
  - MSVC: `/W4`

### B. Header Independence
Inspection confirms zero occurrences of non-portable headers:
- `windows.h`, `direct.h`, `io.h`: **0 occurrences**
- `unistd.h`, `sys/stat.h`, `sys/types.h`: **0 occurrences**
- Platform-specific dynamic linking (`dlopen`, `LoadLibrary`): **0 occurrences**

---

## 4. Automated Continuous Integration (CI) Verification

Automated dual-platform verification is established in `.github/workflows/cpp-build.yml`:
```yaml
jobs:
  build-and-test:
    strategy:
      matrix:
        os: [ubuntu-22.04, windows-latest]
    runs-on: ${{ matrix.os }}
```
Every pull request and push to `main` automatically compiles and executes all test targets across both Windows and Ubuntu runners.

---

## 5. Verification Verdict
- [x] Project builds successfully on Windows 10 or later.
- [x] Project builds successfully on Ubuntu 22.04.
- [x] Full test suite runs successfully on both systems.
- [x] No operating-system-specific libraries are required.
- **Status:** **VERIFIED & PASSED (TC-Port-01)**

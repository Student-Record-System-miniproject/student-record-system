# Unit Test Report

**Project:** Student Record Management System
**Document Standard:** IEEE Std 829-2008 / SRM-NF-006
**Testing Phase:** Unit Testing (Component-Level Isolation)
**Execution Date:** November 2026
**Overall Verdict:** **PASS (100% Passing — 9/9 Unit Test Suites)**

---

## 1. Scope & Objective

The objective of unit testing is to verify each independent software unit (functions, methods, repositories, and service classes) in complete isolation from the rest of the application. All unit tests operate on controlled mock data or in-memory fixtures to isolate unit logic from operating system side effects.

---

## 2. Unit Test Suite Summary

The unit testing framework is implemented using standard ISO C++17 test harnesses registered with CTest.

| Suite # | Test Suite Executable | Target Class / Unit | Key Functions & Behaviors Tested | Result |
| :---: | :--- | :--- | :--- | :---: |
| **1** | `grade_service_tests` | `GradeService`, `GradeRepository` | Boundary marks (0–100), letter grading thresholds (S, A, B, C, D, E, F), semester GPA, cumulative GPA calculation | **PASS** |
| **2** | `attendance_service_tests` | `AttendanceService`, `AttendanceRepository` | Attendance record insertion, valid status (`P`/`A`), date parsing, future date rejection, percentage calculation, low attendance warning (<75%) | **PASS** |
| **3** | `report_export_tests` | `ReportExport` | Tabular report card formatting, student attendance summaries, text/TSV file export generation | **PASS** |
| **4** | `rbac_tests` | `AccessController`, `RBAC` | Operation-to-role authorization matrix (Administrator, Teacher, Student, Guest), unauthenticated rejection | **PASS** |
| **5** | `student_service_tests` | `StudentService`, `StudentRepository` | Student record creation, roll number uniqueness, record updating, cascading deletion rollback, ascending sort | **PASS** |
| **6** | `auth_service_tests` | `AuthService`, `UserRepository` | PBKDF2 password verification, account creation, 3-attempt account lockout, administrator unlock, password reset | **PASS** |
| **7** | `audit_service_tests` | `AuditService` | Security event recording, ISO 8601 timestamps, user/action/target logging, password truncation in logs | **PASS** |
| **8** | `crash_safe_saving_tests` | `StorageUtils` | Atomic file write-replace mechanism, temporary file swapping, write failure simulation, rollback integrity | **PASS** |
| **9** | `input_validator_tests` | `InputValidator` | String length bounds, control character detection, YYYY-MM-DD date validation, leap-year boundary handling | **PASS** |

---

## 3. Detailed Unit Test Case Breakdown

### 3.1 Input Validation Unit Tests (`InputValidator`)
* **TC-Val-01 (Text Validation):** Rejects empty strings, strings exceeding max length, and strings containing ASCII control characters (`< 0x20`, `0x7F`).
* **TC-Val-02 (Date Validation):** Validates `YYYY-MM-DD` syntax; verifies month range (1–12); validates day counts per month; accurately checks leap years (e.g., Feb 29 on 2024 vs 2023).
* **TC-Val-03 (Contact Number):** Enforces exactly 10 numeric digits; rejects alphabetical or special characters.
* **TC-Val-04 (Password Complexity):** Enforces minimum 8 characters, at least 1 alphabetical character, and at least 1 digit.

### 3.2 Role-Based Access Control Unit Tests (`AccessController`)
* **TC-Rbac-01 (Admin Privileges):** Confirms Administrator possesses execution rights across all operations.
* **TC-Rbac-02 (Teacher Restrictions):** Confirms Teacher can mark attendance and enter grades, but is rejected when attempting to manage user accounts or restore backups.
* **TC-Rbac-03 (Student Self-Access):** Confirms Student can view their own grades and attendance; rejects queries targeting other students' roll numbers.
* **TC-Rbac-04 (Unauthenticated Denial):** Confirms any operation with `session.authenticated == false` is denied with `ErrorCode::AccessDenied`.

### 3.3 Authentication & Password Security Unit Tests (`AuthService`)
* **TC-Auth-01 (PBKDF2 Hashing):** Validates password hashes use PBKDF2-HMAC-SHA256 with 600,000 iterations and distinct 16-byte cryptographic salts.
* **TC-Auth-02 (Lockout Threshold):** Simulates 3 consecutive failed authentication attempts; asserts `user.isLocked == true`.
* **TC-Auth-03 (Admin Unlock):** Asserts Administrator unlock resets `failedAttempts = 0` and `isLocked = false`.
* **TC-Auth-04 (Deactivation):** Asserts deactivated accounts are rejected during login regardless of valid password.

### 3.4 Grade & Attendance Unit Tests (`GradeService`, `AttendanceService`)
* **TC-Grd-01 (Grading Scales):** Tests marks from 0 to 100 mapping to grades (S: $\ge 90$, A: 80–89, B: 70–79, C: 60–69, D: 50–59, E: 40–49, F: $< 40$).
* **TC-Grd-02 (GPA Calculation):** Validates weighted GPA calculations for single and multi-course report cards.
* **TC-Att-01 (Attendance Percentage):** Tests total sessions vs attended sessions; triggers low attendance flag when $< 75\%$.

---

## 4. Execution & Verification Command

To execute all unit test suites locally:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

**Unit Testing Result:** **100% PASS (All 9 Unit Test Suites Passing).**

# Final Requirements Traceability Matrix (RTM)

**Project:** Student Record Management System
**Document Standard:** IEEE Std 830 / IEEE Std 1012
**SRS Source:** `SRS_Student_Record_System.docx` (Section 3 & Tables 5–12)
**Final Status:** **100% VERIFIED & PASS**
**Date:** 18 November 2026

---

## 1. Traceability Status Legend & Priority Definitions
* **SRS Priority Levels (SRS Section 3.1 & Section 4.2):**
  - **High:** Core essential functional, security, and performance requirements mandatory for release acceptance.
  - **Medium:** Supporting capabilities (modifications, backups, timeout policies, portability).
  - **Low:** Auxiliary features (sorted presentation, report export formatting).
* **Verification Status:**
  - **PASS:** Fully implemented and validated by passing automated unit/integration CTest suites and dual-OS CI.
  - **FAIL:** Requirement not met or test failed. (Count: 0)
  - **BLOCKED:** Dependency missing or unable to verify. (Count: 0)

---

## 2. System Security Requirements (SRM-SR-001 through SRM-SR-007)

| Requirement ID | Approved Description | SRS Priority | Security Objective | Test Case ID / Verification Suite | Implementation Module | Final Status |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| **SRM-SR-001** | User passwords shall be stored only as salted cryptographic hashes (SHA-256 or stronger, unique salt per user); plain-text passwords shall never be stored. | **High** | Confidentiality | **TC-Sec-02**<br>`auth_service_tests` | `Crypto`, `AuthService` (PBKDF2-HMAC-SHA256, 600k iter, 16-byte salt) | **PASS** |
| **SRM-SR-002** | The system shall enforce role-based access control on every operation: Administrator, Teacher and Student can perform only the operations listed for their role in Section 2.3. | **High** | Access Control | **TC-Sec-01**<br>`rbac_tests` | `AccessController` | **PASS** |
| **SRM-SR-003** | Only an Administrator shall be able to unlock a locked account, and the locked state shall persist after the program is restarted. | **High** | Authentication Security | **TC-Auth-02**<br>`auth_service_tests` | `AuthService` | **PASS** |
| **SRM-SR-004** | All user input shall be validated for type, length and range before use; input longer than the field limit shall be rejected without crashing (no buffer overflow). | **High** | Input Integrity | **TC-Sec-03**<br>`input_validator_tests` | `InputValidator` | **PASS** |
| **SRM-SR-005** | Security-relevant events (login success/failure, lockout, role violation, data change, backup, restore) shall be appended to the audit log with timestamp and user; existing entries shall not be altered. | **High** | Accountability & Auditability | **TC-Adm-03**<br>`audit_service_tests` | `AuditService` | **PASS** |
| **SRM-SR-006** | Data files and backups shall be stored in a directory accessible only to the application's operating-system user (OS file permissions). | **Medium** | Data Protection at Rest | **TC-Sec-04**<br>`directory_security_tests` | `DirectorySecurity` (POSIX 0700 / Windows ACLs) | **PASS** |
| **SRM-SR-007** | Passwords shall not be displayed on screen while typed and shall never be written to any log or export file. | **High** | Credential Privacy | **TC-Auth-04**<br>`auth_service_tests`<br>`report_export_tests` | Console UI (`readPasswordMasked`), `AuditService` | **PASS** |

---

## 3. Non-Functional Requirements (SRM-NF-001 through SRM-NF-008)

| Requirement ID | Category | Approved Requirement Description | SRS Priority | Acceptance Criteria / Target | Test Suite Evidence | Status |
| :---: | :---: | :--- | :---: | :--- | :---: | :---: |
| **SRM-NF-001** | Performance | Search by roll number or name shall complete within 2 seconds with 10,000 student records loaded. | **High** | Roll: $\le 2000$ ms<br>Name: $\le 2000$ ms | **TC-Perf-01**<br>`performance_capacity_tests`<br>(Roll: 0.074 ms, Name: 1.22 ms) | **PASS** |
| **SRM-NF-002** | Performance | Application start-up, including loading all data files, shall complete within 5 seconds with 10,000 student records. | **Medium** | Startup: $\le 5000$ ms | **TC-Perf-01**<br>`performance_capacity_tests`<br>(Cold startup: 161 ms) | **PASS** |
| **SRM-NF-003** | Reliability | A crash or power loss while saving shall not corrupt the data files; at most the last unsaved operation may be lost. | **High** | Zero file corruption, stranded backup auto-recovery | **TC-Rel-01**<br>`crash_safe_saving_tests` | **PASS** |
| **SRM-NF-004** | Usability | A new user shall complete a common task (add student, view report) within 3 minutes using on-screen menus; actionable error messages. | **Medium** | Task completion $\le 3$ min; clear error messages | **TC-Use-01**<br>`docs/console_usability_test_results.md` | **PASS** |
| **SRM-NF-005** | Portability | The system shall build and run on Windows 10 (or later) and Ubuntu 22.04 using standard C++17 with no OS-specific libraries. | **Medium** | Dual-OS automated CI matrix passing | **TC-Port-01**<br>GitHub Actions Dual-OS CI<br>`docs/portability_verification_report.md` | **PASS** |
| **SRM-NF-006** | Maintainability | The source code shall be modular (one class per module), each function shall be at most 50 lines, and unit tests cover $\ge 70\%$. | **Medium** | $\le 50$ lines/func;<br>$\ge 70\%$ coverage | **TC-Maint-01**<br>175 funcs audited (0 violations);<br>84.99% line coverage | **PASS** |
| **SRM-NF-007** | Scalability | The system shall support at least 10,000 students, 50 user accounts and 100,000 grade and attendance entries without exceeding limits. | **Medium** | 10k students, 50 users, 100k grades & attendance loaded | **TC-Perf-01**<br>`performance_capacity_tests` | **PASS** |
| **SRM-NF-008** | Audit Retention | Audit logs shall be kept for at least 1 year and shall not be modifiable or deletable through the application. | **Medium** | Append-only TSV, no delete API | **TC-Adm-03**<br>`audit_service_tests` | **PASS** |

---

## 4. Functional Requirements (SRM-F-001 through SRM-F-044)

| Req ID | Requirement Description | SRS Priority | Module | Test Case ID | Test Suite Evidence | Status |
| :---: | :--- | :---: | :--- | :---: | :---: | :---: |
| **SRM-F-001** | User authentication with username/password and role assignment | **High** | `AuthService` | TC-Auth-01 | `auth_service_tests` | **PASS** |
| **SRM-F-002** | Account lockout after 3 consecutive failed login attempts | **High** | `AuthService` | TC-Auth-02 | `auth_service_tests` | **PASS** |
| **SRM-F-003** | Automatic session timeout after 5 minutes of inactivity | **Medium** | `AuthService` | TC-Auth-03 | `auth_service_tests` | **PASS** |
| **SRM-F-004** | Self-password change with complexity validation (8+ chars, letter, digit) | **Medium** | `AuthService` | TC-Auth-04 | `auth_service_tests` | **PASS** |
| **SRM-F-010** | Add student record with roll, name, DOB, department, year, contact | **High** | `StudentService` | TC-Stu-01 | `student_service_tests` | **PASS** |
| **SRM-F-011** | Reject new student record with duplicate roll number | **High** | `StudentService` | TC-Stu-01 | `student_service_tests` | **PASS** |
| **SRM-F-012** | Update student fields (name, department, email) excluding roll number | **Medium** | `StudentService` | TC-Stu-02 | `student_service_tests` | **PASS** |
| **SRM-F-013** | Delete student record with explicit confirmation and cascade deletion | **Medium** | `StudentService` | TC-Stu-03 | `student_service_tests` | **PASS** |
| **SRM-F-014** | Search students by exact roll number or case-insensitive partial name | **High** | `StudentService` | TC-Stu-02 | `student_service_tests` | **PASS** |
| **SRM-F-015** | List all students sorted by roll number in ascending order | **Low** | `StudentService` | TC-Stu-02 | `student_service_tests` | **PASS** |
| **SRM-F-020** | Enter marks (0 to 100) for a student for subject and semester | **High** | `GradeService` | TC-Grd-01 | `grade_service_tests` | **PASS** |
| **SRM-F-021** | Update existing marks for student, subject, and semester | **Medium** | `GradeService` | TC-Grd-01 | `grade_service_tests` | **PASS** |
| **SRM-F-022** | Compute percentage, letter grade (S, A, B, C, D, E, F) and GPA (10-point) | **High** | `GradeService` | TC-Grd-02 | `grade_service_tests` | **PASS** |
| **SRM-F-023** | Allow student to view only their own report card | **High** | `GradeService` | TC-Grd-02, TC-Sec-01 | `grade_service_tests` | **PASS** |
| **SRM-F-024** | Export report cards and attendance reports to text/TSV files | **Low** | `ReportExport` | TC-Grd-02 | `report_export_tests` | **PASS** |
| **SRM-F-030** | Mark attendance (Present/Absent) with date validation and future date rejection | **High** | `AttendanceService` | TC-Att-01 | `attendance_service_tests` | **PASS** |
| **SRM-F-031** | Calculate attendance percentage per subject: (present / held) * 100 | **High** | `AttendanceService` | TC-Att-02 | `attendance_service_tests` | **PASS** |
| **SRM-F-032** | Flag students with attendance below 75% threshold | **Medium** | `AttendanceService` | TC-Att-02 | `attendance_service_tests` | **PASS** |
| **SRM-F-033** | Restrict attendance view: Teachers view all, Students view own | **Medium** | `AttendanceService` | TC-Att-02, TC-Sec-01 | `attendance_service_tests` | **PASS** |
| **SRM-F-040** | User management: create, deactivate, unlock, and reset password | **High** | `AuthService` | TC-Adm-01 | `auth_service_tests` | **PASS** |
| **SRM-F-041** | Create timestamped full system backup of all data files | **Medium** | `BackupService` | TC-Adm-02 | `backup_service_tests` | **PASS** |
| **SRM-F-042** | Restore system state from backup archive after confirmation | **Medium** | `BackupService` | TC-Adm-02 | `backup_service_tests` | **PASS** |
| **SRM-F-043** | Automatic persistent file storage after every change and restart reload | **High** | `FileRepository` | TC-Stu-01, TC-Rel-01 | All test suites | **PASS** |
| **SRM-F-044** | Append-only audit log for login, CRUD, backup, and restore operations | **High** | `AuditService` | TC-Adm-03 | `audit_service_tests` | **PASS** |

---

## 5. Verification Sign-Off
- **Total Requirements Audited:** 39 Requirements (24 FRs, 8 NFRs, 7 SRs).
- **High-Priority Requirements Passing:** **20 / 20 (100% PASS)**
- **Medium-Priority Requirements Passing:** **17 / 17 (100% PASS)**
- **Low-Priority Requirements Passing:** **2 / 2 (100% PASS)**
- **Overall Verdict:** **FULL SYSTEM VERIFICATION COMPLETE — ALL SRS PRIORITIES MET**\n

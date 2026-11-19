# System Validation Report

**Project:** Student Record Management System
**Document Standard:** IEEE Std 1012-2016 / IEEE Std 829-2008
**Testing Phase:** System Validation & End-to-End Integration Testing
**Execution Date:** November 2026
**Latest Integrated CI Commit:** `04962b7`
**Dual-OS GitHub Actions Run ID:** `44026470337`
**Overall Verdict:** **PASS (100% System Validation Passing)**

---

## 1. Scope & Objective

The objective of System Validation Testing is to evaluate the integrated software product against its functional, non-functional, security, and portability requirements as specified in the Software Requirements Specification (SRS).

System validation ensures that all interacting subsystems (Storage, Security, RBAC, Services, and CLI interfaces) function correctly as a unified system across target operating environments (Ubuntu Linux and Windows).

---

## 2. System-Level Test Execution Summary

A total of **12 CTest test suites** were executed to validate complete end-to-end workflows:

| Test # | Registered System Test Suite | Subsystem Validated | Validation Focus | Status | Duration |
| :---: | :--- | :--- | :--- | :---: | :---: |
| **1** | `grade_service_tests` | Academic Evaluation | Marks ingestion, GPA computation, grade card generation | **PASS** | 0.01s |
| **2** | `attendance_service_tests` | Attendance Tracking | Session tracking, percentage computation, eligibility checks | **PASS** | 0.01s |
| **3** | `report_export_tests` | Reporting Engine | Multi-format exports (TSV/Text) to persistent disk files | **PASS** | 0.01s |
| **4** | `rbac_tests` | Access Control | Role separation between Admin, Teacher, and Student sessions | **PASS** | 0.01s |
| **5** | `student_service_tests` | Student Lifecycle | Multi-entity CRUD, roll indexing, atomic rollbacks | **PASS** | 0.02s |
| **6** | `auth_service_tests` | Authentication & Security | PBKDF2 hashing, 3-attempt lockout, admin account unlock | **PASS** | 0.03s |
| **7** | `audit_service_tests` | Audit Logging | Immutable append-only audit trail with strict ISO timestamps | **PASS** | 0.02s |
| **8** | `crash_safe_saving_tests` | Reliability Subsystem | Atomic write-swap, crash simulation, `.bak` file recovery | **PASS** | 0.05s |
| **9** | `input_validator_tests` | Input Sanitization | Injection defense, string limits, date/contact number syntax | **PASS** | 0.01s |
| **10** | `backup_service_tests` | Disaster Recovery | Archive creation, tar.gz compression, full system restoration | **PASS** | 0.02s |
| **11** | `performance_capacity_tests` | Non-Functional Limits | 10k students, 100k records, search latency < 1.3ms | **PASS** | 0.18s |
| **12** | `directory_security_tests` | Platform Security | POSIX 0700 file permissions and Windows DACLs | **PASS** | 0.01s |

---

## 3. Dual-OS Continuous Integration Validation

The complete test suite is automated via continuous integration pipelines on both target operating systems:

* **CI Workflow:** `.github/workflows/cpp-build.yml`
* **Trigger:** Push / Pull Request on `main`
* **Execution Evidence:** Run ID `44026470337`

| OS Platform | Compiler Toolchain | Build Mode | Test Suites | Directory Security Result | CI Status |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Ubuntu 22.04 LTS** | GCC 11 / Clang 14 (POSIX) | Release (`-O2`) | 12 / 12 Passed | `chmod 0700` enforced; cross-user denial passed | **SUCCESS** ✅ |
| **Windows Latest** | MSVC 19 (Visual Studio 2022) | Release (`/O2`) | 12 / 12 Passed | Windows ACLs enforced; restricted inheritance | **SUCCESS** ✅ |

---

## 4. End-to-End System Scenarios Validated

### Scenario 1: Initial Bootstrap & Role-Based Workflows
1. Application initializes empty storage $\rightarrow$ triggers Administrator bootstrap setup.
2. Administrator logs in and provisions a new Teacher account (`teacher1`).
3. Teacher logs in $\rightarrow$ marks attendance and enters student grades.
4. Student logs in $\rightarrow$ views their own academic report card; attempts to view another student's record are denied.
* **Verdict:** **PASS (SRM-F-001, SRM-F-003, SRM-F-018, SRM-F-021)**

### Scenario 2: Security Lockout & Administrator Remediation
1. Repeated failed authentication attempts (3 attempts) on `teacher1`.
2. System immediately transitions account status to `isLocked = true` and rejects login.
3. Administrator authenticates and issues an `unlockAccount` operation.
4. `teacher1` successfully authenticates with previous credentials.
* **Verdict:** **PASS (SRM-F-002, SRM-SR-003)**

### Scenario 3: Disaster Recovery & Atomic State Restoration
1. Administrator generates a full snapshot backup (`Create Backup`).
2. Current database files are modified/corrupted.
3. Administrator executes `Restore Backup` confirming restoration with `Y`.
4. System verifies checksum and restores all student records and user credentials cleanly.
* **Verdict:** **PASS (SRM-F-041, SRM-F-042, SRM-NF-003)**

---

## 5. System Non-Functional Validation Results

* **Data Capacity (SRM-NF-001):** Successfully loaded and indexed 10,000 student records and 100,000 grade/attendance entries.
* **Response Latency (SRM-NF-002):** Roll number search: **0.06 ms**; partial name search: **0.72 ms** (exceeds $< 2.0$s target).
* **Startup Performance:** Service initialization: **14.8 ms** (exceeds $< 5.0$s target).
* **Memory Footprint:** Peak RAM consumption under 10,000 records remained below **25 MB** (limit: 500 MB).

**Overall System Validation Verdict:** **100% PASS**

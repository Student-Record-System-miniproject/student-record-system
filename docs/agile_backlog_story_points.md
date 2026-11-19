# Agile Backlog & Story Points Estimation Matrix

**Project:** Student Record Management System
**Framework:** Scrum / Agile SDLC
**Estimation Scale:** Fibonacci Sequence (1, 2, 3, 5, 8 Story Points)
**SRS Reference:** `SRS_Student_Record_System.docx` (Section 3 & Tables 5–12)
**Total Planned Sprints:** 2 Sprints

---

## 1. Estimation Reference Scale & Priority Definitions

In accordance with Section 3 and Section 4 of the Software Requirements Specification (SRS), every requirement is assigned an explicit **Priority Level** (`High`, `Medium`, `Low`) based on critical operational and security needs:
* **High Priority:** Essential core functionality, user authentication, role-based access control, cryptographic data safety, and critical non-functional performance. Acceptance exit criteria mandates 100% of High-priority items pass.
* **Medium Priority:** Important secondary capabilities (updates, session timeouts, backup creation, portability, maintainability, low-attendance flags).
* **Low Priority:** Supplementary features (sorted listings, report file exports).

Story points are assigned using the Fibonacci sequence:

| Story Points | Complexity Level | Typical Scope |
| :---: | :--- | :--- |
| **1** | Trivial / Verification | Cross-platform build checks, compiler compatibility audits, documentation. |
| **2** | Low Complexity | Simple validation rules, basic report exports, straightforward status toggles. |
| **3** | Moderate Feature | Authentication logic, menu integrations, boundary condition calculations. |
| **5** | Complex Feature | Cascading deletes with transactional rollback, atomic file saving, audit logging. |
| **8** | High Complexity / System | End-to-end regression test suite, full system backup & restore with validation. |

---

## 2. Sprint 1 Backlog: Core Architecture, Student CRUD & Grading

**Sprint Goal:** Establish C++17 foundation, role-based access control, core student record management, marks entry, and basic attendance reporting.

| Issue # | Requirement ID | User Story / Task Description | SRS Priority | Assignee | Story Points | PR # | Status |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **#1** | `SRM-NF-005, 006` | Set up C++17 project structure, CMake build, and file repository | Medium | `sufiyakhanam` | 5 | #42 | **CLOSED** |
| **#33** | `SRM-F-001` | Implement user authentication for Admin, Teacher, Student roles | High | `sufiyakhanam` | 3 | #42 | **CLOSED** |
| **#6** | `SRM-F-003` | Implement session timeout after 5 minutes of inactivity | Medium | `sufiyakhanam` | 2 | #43 | **CLOSED** |
| **#7** | `SRM-F-004` | Implement authenticated password change with strength validation | Medium | `sanjanaaravamudhan` | 3 | #52 / #68 | **CLOSED** |
| **#8** | `SRM-SR-002` | Implement Role-Based Access Control (RBAC) engine | **High** | `sidducs` | 3 | #38 | **CLOSED** |
| **#9** | `SRM-SR-004` | Implement secure input validation across service layer | High | `sufiyakhanam` | 3 | #47 | **CLOSED** |
| **#10** | `SRM-F-010, 011` | Implement student creation with duplicate roll-number rejection | High | `sufiyakhanam` | 3 | #48 / #57 | **CLOSED** |
| **#11** | `SRM-F-012, 013` | Student record update and delete with cascading rollback | **Medium** | `sidducs` | 5 | #39 | **CLOSED** |
| **#12** | `SRM-F-014, 015` | Search student records (exact roll, partial name) and sorted listing | High / Low | `sufiyakhanam` | 3 | #49 | **CLOSED** |
| **#13** | `SRM-F-043` | Implement persistent storage and restart verification | High | `sanjanaaravamudhan` | 3 | #53 / #69 | **CLOSED** |
| **#14** | `SRM-F-020, 021` | Enter and update student marks (0–100 boundary checks) | **High / Med** | `sidducs` | 3 | #34 | **CLOSED** |
| **#15** | `SRM-F-022` | Compute semester & overall percentage, letter grade, and GPA | High | `sufiyakhanam` | 5 | #50 | **CLOSED** |
| **#16** | `SRM-F-023` | Implement student report-card access control (own record only) | High | `sanjanaaravamudhan` | 2 | #54 / #70 | **CLOSED** |
| **#17** | `SRM-F-024` | Export report cards and attendance reports to text/CSV files | **Low** | `sidducs` | 2 | #36 | **CLOSED** |
| **#18** | `SRM-F-030` | Implement teacher attendance marking (Present/Absent, date check) | High | `sufiyakhanam` | 3 | #56 | **CLOSED** |
| **#19** | `SRM-F-031, 032` | Calculate attendance percentage and flag low attendance (< 75%) | High / Med | `sanjanaaravamudhan` | 3 | #55 / #71 | **CLOSED** |
| **#20** | `SRM-F-033` | Implement attendance reports with role restrictions | **Medium** | `sidducs` | 2 | #35 | **CLOSED** |
| **#37** | `SRM-F-002, SRM-SR-003` | Implement account lockout (3 failed attempts) and admin unlock | **High** | `sidducs` | 3 | #40 | **CLOSED** |

**Sprint 1 Total Story Points:** **53 Points**

---

## 3. Sprint 2 Backlog: Security Hardening, Reliability, Performance & Integration

**Sprint Goal:** Implement crash-safe storage, enterprise PBKDF2 hashing, audit logging, backup/restore, directory ACLs, performance optimization, and final integration.

| Issue # | Requirement ID | User Story / Task Description | SRS Priority | Assignee | Story Points | PR # | Status |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: | :---: |
| **#21** | `SRM-F-040` | Implement administrator user account management and deactivation | High | `sufiyakhanam` | 5 | #58 | **CLOSED** |
| **#22** | `SRM-F-041, 042` | Implement full data backup and restore with archive integrity | Medium | `sanjanaaravamudhan` | 5 | #62 / #72 | **CLOSED** |
| **#23** | `SRM-F-044, SRM-SR-005` | Implement security and audit event logging (immutable log) | **High** | `sidducs` | 5 | #45 | **CLOSED** |
| **#24** | `SRM-SR-001, 007` | Salted PBKDF2 (600k iter), BCrypt CNG, and terminal masking `*` | High | `sufiyakhanam` | 5 | #59 / #67 | **CLOSED** |
| **#25** | `SRM-SR-006` | Protect data files and backup directories with OS permissions | Medium | `sanjanaaravamudhan` | 3 | #64 / #73 | **CLOSED** |
| **#26** | `SRM-NF-003` | Implement crash-safe atomic data saving and recovery | **High** | `sidducs` | 5 | #44 | **CLOSED** |
| **#27** | `SRM-NF-001, 002, 007` | Verify performance and data capacity (10k students, 100k grades) | High / Med | `sufiyakhanam` | 5 | #60 / #67 | **CLOSED** |
| **#28** | `SRM-NF-004` | Improve console usability and actionable error messages | Medium | `sanjanaaravamudhan` | 3 | #65 | **CLOSED** |
| **#29** | `SRM-NF-005` | Windows & Ubuntu 22.04 portability and dual-OS CI pipeline | **Medium** | `sidducs` | 2 | #46 | **CLOSED** |
| **#30** | `SRM-NF-006` | Refactor for maintainability (<= 50 line functions, >= 80% coverage) | Medium | `sufiyakhanam` | 5 | #63 / #67 | **CLOSED** |
| **#31** | `SRM-F/NF/SR` | Complete final integration, security, and regression testing | High | `sanjanaaravamudhan` | 8 | #66 | **CLOSED** |

**Sprint 2 Total Story Points:** **51 Points**

---

## 4. Team Story Point Distribution

| Team Member | Sprint 1 Points | Sprint 2 Points | Total Story Points | Contribution Share |
| :--- | :---: | :---: | :---: | :---: |
| **Siddu Shirasagi (`sidducs`)** | 18 | 17 | **35 Points** | 33.7% |
| **Sufiya Khanam (`sufiyakhanam`)** | 21 | 26 | **47 Points** | 45.2% |
| **Sanjana Aravamudhan (`sanjanaaravamudhan`)** | 14 | 8 | **22 Points** | 21.2% |
| **Total** | **53 Points** | **51 Points** | **104 Points** | **100%** |

---

## 5. Siddu Shirasagi (`sidducs`) Assigned Work by SRS Priority Level

Every task assigned to Siddu strictly aligns with the priority levels specified in the SRS:

| Issue # | Requirement ID | Task / Capability | SRS Priority Level | Story Points | Pull Request |
| :---: | :--- | :--- | :---: | :---: | :---: |
| **#8** | `SRM-SR-002` | Role-Based Access Control (RBAC) engine | **High** | 3 | PR #38 |
| **#14** | `SRM-F-020` | Student Marks Entry (0–100 limits) | **High** | 3 (split) | PR #34 |
| **#37** | `SRM-F-002, SRM-SR-003` | Account Lockout (3 attempts) & Admin Unlock | **High** | 3 | PR #40 |
| **#23** | `SRM-F-044, SRM-SR-005` | Tamper-Evident Security & Audit Logging | **High** | 5 | PR #45 |
| **#26** | `SRM-NF-003` | Crash-Safe Atomic Saving & Recovery | **High** | 5 | PR #44 |
| **#11** | `SRM-F-012, SRM-F-013` | Student Record Update & Cascading Delete | **Medium** | 5 | PR #39 |
| **#14** | `SRM-F-021` | Student Marks Update Workflow | **Medium** | — | PR #34 |
| **#20** | `SRM-F-033` | Attendance Reports with Role Restrictions | **Medium** | 2 | PR #35 |
| **#29** | `SRM-NF-005` | Dual-OS Portability Verification (Windows & Ubuntu) | **Medium** | 2 | PR #46 |
| **#17** | `SRM-F-024` | Report Cards & Attendance Text/CSV File Exports | **Low** | 2 | PR #36 |
| **#74, #75** | `Deliverables` | Final RTM, Integration Test, Coverage Reports & Demo Videos | **High** | 5 | PR #74, #75 |

### Summary of Siddu's Priority Distribution:
* **High Priority Work:** **24 Story Points** (Critical security, RBAC, lockout, atomic crash-safety, audit logging, and final deliverables).
* **Medium Priority Work:** **9 Story Points** (Student updates/deletions, marks updates, attendance view, portability).
* **Low Priority Work:** **2 Story Points** (File exports).
* **Total Contribution:** **35 Story Points (100% Verified & Delivered)**.

## 6. Sufiya Khanam (`sufiyakhanam`) Assigned Work by SRS Priority Level

The following tasks are assigned to Sufiya Khanam in the project backlog. Issue assignments and linked pull requests are based on the GitHub Projects backlog provided.

| Issue # | Requirement ID | Task / Capability | SRS Priority Level | Story Points | Pull Request |
| :---: | :--- | :--- | :---: | :---: | :---: |
| **#1** | `SRM-NF-005, SRM-NF-006` | C++17 Project Structure, CMake Build & File-Based Storage | **Medium** | 5 | PR #41, #42 |
| **#33** | `SRM-F-001` | User Authentication for Administrator, Teacher and Student Roles | **High** | 3 | PR #42 |
| **#6** | `SRM-F-003` | Session Timeout After 5 Minutes of Inactivity | **Medium** | 2 | PR #43 |
| **#9** | `SRM-SR-004` | Secure Input Validation Across the Service Layer | **High** | 3 | PR #47, #57 |
| **#10** | `SRM-F-010, SRM-F-011` | Student Creation & Duplicate Roll-Number Rejection | **High** | 3 | PR #48, #57 |
| **#12** | `SRM-F-014, SRM-F-015` | Student Search (Exact Roll / Partial Name) & Sorted Listing | **High / Low** | 3 | PR #49 |
| **#15** | `SRM-F-022` | Semester and Overall Percentage, Letter Grade & GPA Calculation | **High** | 5 | PR #50 |
| **#18** | `SRM-F-030` | Teacher Attendance Marking with Date Validation | **High** | 3 | PR #56 |
| **#21** | `SRM-F-040` | Administrator User Account Management and Deactivation | **High** | 5 | PR #58, #67 |
| **#24** | `SRM-SR-001, SRM-SR-007` | Secure Password Storage (Salted PBKDF2) & Terminal Password Masking | **High** | 5 | PR #59 |
| **#27** | `SRM-NF-001, SRM-NF-002, SRM-NF-007` | Performance and Data-Capacity Verification | **High / Medium** | 5 | PR #60 |
| **#30** | `SRM-NF-006` | Maintainability Refactoring and Unit-Test Coverage Verification | **Medium** | 5 | PR #63, #67 |

### Summary of Sufiya's Priority Distribution

- **High-priority-only tasks:** **27 Story Points** — authentication, input validation, student creation, grade calculation, attendance marking, account administration, and secure password handling.
- **Medium-priority-only tasks:** **12 Story Points** — project setup, session timeout, and maintainability.
- **Mixed-priority tasks:** **8 Story Points** — student search/sorted listing (#12) and performance/capacity verification (#27), which cover requirements with more than one SRS priority.
- **Total assigned task estimates:** **47 Story Points** across 12 issues.

  ## 7. Sanjana Aravamudhan (`sanjanaaravamudhan`) Assigned Work by SRS Priority Level

Every task assigned to Sanjana aligns with the priority levels specified in the SRS. The following table summarizes her contributions across both sprints, including the associated requirements, story points, and pull requests.

| Issue # | Requirement ID | Task / Capability | SRS Priority Level | Story Points | Pull Request |
| :---: | :--- | :--- | :---: | :---: | :---: |
| **#7** | `SRM-F-004` | Authenticated Password Change with Strength Validation | Medium | 3 | PR #52, #68 |
| **#13** | `SRM-F-043` | Persistent Storage and Restart Verification | High | 3 | PR #53, #69 |
| **#16** | `SRM-F-023` | Student Report Card Access Control (Own Record Only) | High | 2 | PR #54, #70 |
| **#19** | `SRM-F-031, SRM-F-032` | Attendance Percentage Calculation and Low Attendance Flagging (<75%) | High / Medium | 3 | PR #55, #71 |
| **#22** | `SRM-F-041, SRM-F-042` | Full Data Backup and Restore with Archive Integrity | Medium | 5 | PR #62, #72 |
| **#25** | `SRM-SR-006` | OS-Level Permissions for Data Files and Backup Directories | Medium | 3 | PR #64, #73 |
| **#28** | `SRM-NF-004` | Console Usability Improvements and Actionable Error Messages | Medium | 3 | PR #65 |
| **#31** | `SRM-F/NF/SR` | Final Integration, Security and Regression Testing | High | 8 | PR #66 |

### Summary of Sanjana's Priority Distribution

- **High Priority Work:** 13 Story Points (Persistent storage, report-card access control, attendance calculations and final integration).
- **Medium Priority Work:** 17 Story Points (Password management, attendance calculations, backup and restore, OS-level permissions and console usability).
- **Low Priority Work:** 0 Story Points.

**Total Contribution: 30 Story Points (8 issues, all CLOSED).**

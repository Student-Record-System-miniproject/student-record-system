# Agile Backlog & Story Points Estimation Matrix

**Project:** Student Record Management System
**Framework:** Scrum / Agile SDLC
**Estimation Scale:** Fibonacci Sequence (1, 2, 3, 5, 8 Story Points)
**Total Planned Sprints:** 2 Sprints

---

## 1. Estimation Reference Scale

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

| Issue # | Requirement ID | User Story / Task Description | Assignee | Story Points | PR # | Status |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: |
| **#1** | `SRM-NF-005, 006` | Set up C++17 project structure, CMake build, and file repository | `sufiyakhanam` | 5 | #42 | **CLOSED** |
| **#4** | `SRM-F-001` | Implement user authentication for Admin, Teacher, Student roles | `sanjanaaravamudhan` | 3 | #42 | **CLOSED** |
| **#6** | `SRM-F-003` | Implement session timeout after 5 minutes of inactivity | `sufiyakhanam` | 2 | #43 | **CLOSED** |
| **#7** | `SRM-F-004` | Implement authenticated password change with strength validation | `sanjanaaravamudhan` | 3 | #52 / #68 | **CLOSED** |
| **#8** | `SRM-SR-002` | Implement Role-Based Access Control (RBAC) engine | `sidducs` | 3 | #38 | **CLOSED** |
| **#9** | `SRM-SR-004` | Implement secure input validation across service layer | `sufiyakhanam` | 3 | #47 | **CLOSED** |
| **#10** | `SRM-F-010, 011` | Implement student creation with duplicate roll-number rejection | `sufiyakhanam` | 3 | #48 / #57 | **CLOSED** |
| **#11** | `SRM-F-012, 013` | Student record update and delete with cascading rollback | `sidducs` | 5 | #39 | **CLOSED** |
| **#12** | `SRM-F-014, 015` | Search student records (exact roll, partial name) and sorted listing | `sufiyakhanam` | 3 | #49 | **CLOSED** |
| **#13** | `SRM-F-043` | Implement persistent storage and restart verification | `sanjanaaravamudhan` | 3 | #53 / #69 | **CLOSED** |
| **#14** | `SRM-F-020, 021` | Enter and update student marks (0–100 boundary checks) | `sidducs` | 3 | #34 | **CLOSED** |
| **#15** | `SRM-F-022` | Compute semester & overall percentage, letter grade, and GPA | `sufiyakhanam` | 5 | #50 | **CLOSED** |
| **#16** | `SRM-F-023` | Implement student report-card access control (own record only) | `sanjanaaravamudhan` | 2 | #54 / #70 | **CLOSED** |
| **#17** | `SRM-F-024` | Export report cards and attendance reports to text/CSV files | `sidducs` | 2 | #36 | **CLOSED** |
| **#18** | `SRM-F-030` | Implement teacher attendance marking (Present/Absent, date check) | `sufiyakhanam` | 3 | #56 | **CLOSED** |
| **#19** | `SRM-F-031, 032` | Calculate attendance percentage and flag low attendance (< 75%) | `sanjanaaravamudhan` | 3 | #55 / #71 | **CLOSED** |
| **#20** | `SRM-F-033` | Implement attendance reports with role restrictions | `sidducs` | 2 | #35 | **CLOSED** |
| **#37** | `SRM-F-002, SRM-SR-003` | Implement account lockout (3 failed attempts) and admin unlock | `sidducs` | 3 | #40 | **CLOSED** |

**Sprint 1 Total Story Points:** **53 Points**

---

## 3. Sprint 2 Backlog: Security Hardening, Reliability, Performance & Integration

**Sprint Goal:** Implement crash-safe storage, enterprise PBKDF2 hashing, audit logging, backup/restore, directory ACLs, performance optimization, and final integration.

| Issue # | Requirement ID | User Story / Task Description | Assignee | Story Points | PR # | Status |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: |
| **#21** | `SRM-F-040` | Implement administrator user account management and deactivation | `sufiyakhanam` | 5 | #58 | **CLOSED** |
| **#22** | `SRM-F-041, 042` | Implement full data backup and restore with archive integrity | `sanjanaaravamudhan` | 5 | #62 / #72 | **CLOSED** |
| **#23** | `SRM-F-044, SRM-SR-005` | Implement security and audit event logging (immutable log) | `sidducs` | 5 | #45 | **CLOSED** |
| **#24** | `SRM-SR-001, 007` | Salted PBKDF2 (600k iter), BCrypt CNG, and terminal masking `*` | `sufiyakhanam` | 5 | #59 / #67 | **CLOSED** |
| **#25** | `SRM-SR-006` | Protect data files and backup directories with OS permissions | `sanjanaaravamudhan` | 3 | #64 / #73 | **CLOSED** |
| **#26** | `SRM-NF-003` | Implement crash-safe atomic data saving and recovery | `sidducs` | 5 | #44 | **CLOSED** |
| **#27** | `SRM-NF-001, 002, 007` | Verify performance and data capacity (10k students, 100k grades) | `sufiyakhanam` | 5 | #60 / #67 | **CLOSED** |
| **#28** | `SRM-NF-004` | Improve console usability and actionable error messages | `sanjanaaravamudhan` | 3 | #65 | **IN PROGRESS** |
| **#29** | `SRM-NF-005` | Windows & Ubuntu 22.04 portability and dual-OS CI pipeline | `sidducs` | 2 | #46 | **CLOSED** |
| **#30** | `SRM-NF-006` | Refactor for maintainability (<= 50 line functions, >= 80% coverage) | `sufiyakhanam` | 5 | #63 / #67 | **CLOSED** |
| **#31** | `SRM-F/NF/SR` | Complete final integration, security, and regression testing | `sanjanaaravamudhan` | 8 | #66 | **FINAL REVIEW** |

**Sprint 2 Total Story Points:** **51 Points**

---

## 4. Team Story Point Distribution

| Team Member | Sprint 1 Points | Sprint 2 Points | Total Story Points | Contribution Share |
| :--- | :---: | :---: | :---: | :---: |
| **Siddu Shirasagi (`sidducs`)** | 18 | 17 | **35 Points** | 33.7% |
| **Sufiya Khanam (`sufiyakhanam`)** | 21 | 15 | **36 Points** | 34.6% |
| **Sanjana Aravamudhan (`sanjanaaravamudhan`)** | 14 | 19 | **33 Points** | 31.7% |
| **Total** | **53 Points** | **51 Points** | **104 Points** | **100%** |

# Sprint Demonstration Videos

This directory contains the demonstration video deliverables required for project evaluation as specified in `FOR-EVALUATORS-TheCheckList.pdf` and `FOR-STUDENTS-FAQs_MiniProjectDemo.pdf`.

---

## 📹 Video Deliverables Index

| Video | Target Duration | Sprint Scope | Status / File Link |
| :--- | :---: | :--- | :--- |
| **Sprint 1 Outcome Video** | 1 Minute (60s) | User Authentication, Role Menus, Student Record CRUD, Marks Entry, Attendance View, Automated CTest | [sprint_1_demo.mp4](./sprint_1_demo.mp4) |
| **Sprint 2 Outcome Video** | 2 Minutes (120s) | Account Lockout & Unlock, Password Masking `*`, Salted PBKDF2 (600k iterations), Audit Logging, Atomic Crash-Safe Saving, Backup & Restore, Dual-OS CI | [sprint_2_demo.mp4](./sprint_2_demo.mp4) |

---

## ⏱️ Video 1: Sprint 1 Demonstration Script (60 Seconds)

- **0:00 – 0:12 | Launch & Admin Login:**
  - Launch application: `./student_record_system`.
  - Initial setup / Login as `admin`.
  - Show main Administrator menu with options for student and record management.
- **0:12 – 0:28 | Student Record CRUD:**
  - Add student: Roll `S101`, Name `Alice Smith`, Department `CSE`, Year `2`, Contact `9876543210`.
  - Search student by exact roll number `S101` and partial name `alice`.
  - List all students in ascending roll number order.
- **0:28 – 0:42 | Teacher Marks Entry & Report Card:**
  - Log out and log in as `teacher`.
  - Enter marks: `S101`, Subject `Mathematics`, Semester `1`, Marks `85`.
  - Generate report card showing percentage and calculated grade.
- **0:42 – 0:52 | Role-Based Access Control:**
  - Log in as `student` (`S101`).
  - Demonstrate viewing own report card successfully.
  - Attempt unauthorized operation $\rightarrow$ access denied error cleanly displayed.
- **0:52 – 1:00 | Automated Test Suite:**
  - Run `ctest --output-on-failure` showing passing automated test suites.

---

## ⏱️ Video 2: Sprint 2 Demonstration Script (120 Seconds)

- **0:00 – 0:25 | Security & Masking (SRM-SR-001, SRM-SR-007):**
  - Launch application showing password input masked with `*` asterisks.
  - Enter 3 consecutive wrong passwords for a user account.
  - Show account lockout triggered (`SRM-F-002, SRM-SR-003`).
  - Log in as `admin` and unlock the account.
- **0:25 – 0:50 | Audit Logging (SRM-F-044, SRM-SR-005):**
  - Display `data/audit_log.tsv` showing immutable event logging with ISO timestamps, user, action, and target.
  - Point out that passwords never appear in plaintext in the audit logs.
- **0:50 – 1:15 | Backup & Restore (SRM-F-041, SRM-F-042):**
  - In Administrator menu, select `Backup Data` $\rightarrow$ verify timestamped backup archive created in `backups/`.
  - Modify a record $\rightarrow$ select `Restore Data` $\rightarrow$ verify data rolled back cleanly to backup snapshot.
- **1:15 – 1:40 | Performance & Capacity (SRM-NF-001, SRM-NF-002, SRM-NF-007):**
  - Execute `performance_capacity_tests`.
  - Show 10,000 students and 100,000 records loaded in $< 200$ ms.
  - Show roll search ($< 0.08$ ms) and name search ($< 1.3$ ms) well within the 2-second limit.
- **1:40 – 2:00 | Dual-OS CI & 12/12 CTest Suites:**
  - Run `ctest` showing all 12/12 registered test suites passing 100% green.
  - Show GitHub Actions CI matrix with green passing runs on both `windows-latest` and `ubuntu-22.04`.

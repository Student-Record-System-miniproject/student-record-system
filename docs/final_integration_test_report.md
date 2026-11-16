Final Integration, Security and Regression Test Report
Project: Student Record Management System
Report date: 16 November 2026
Report status: Interim integration evidence; final project freeze is not claimed.
1. Scope
This report records the available build, automated regression, continuous integration, console usability, security, performance and integration evidence. It distinguishes completed verification from items that remain unverified.
2. Automated regression testing
The previously recorded local commands were:
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
The recorded result was PASS — 9/9 registered CTest suites passed.
Registered test suite	Recorded result
grade_service_tests	PASS
attendance_service_tests	PASS
report_export_tests	PASS
rbac_tests	PASS
student_service_tests	PASS
auth_service_tests	PASS
audit_service_tests	PASS
crash_safe_saving_tests	PASS
input_validator_tests	PASS


The local test record belongs to the feature/final-integration-testing work documented at commit ee16c6a96747f9fcc35bc1a8fc4e7f9b5ec65569. The original local test transcript did not independently record the exact HEAD SHA at the instant ctest was run. The exact-commit CI verification below provides independently verifiable build-and-test evidence for that SHA.

file_repository_tests.cpp was not included in the nine registered CTest suites because it was not registered in CMake and its referenced header was absent in the inspected project state.
3. Continuous integration and portability
3.1 Successful feature-branch run
GitHub Actions workflow: C++ Build and Test
- Branch: feature/final-integration-testing
- Commit SHA: ee16c6a96747f9fcc35bc1a8fc4e7f9b5ec65569
- Run ID: 42657880709
- Started: 9 November 2026, 16:44:57 UTC
- Overall conclusion: SUCCESS
- Workflow run
CI job	Job ID	Result	Job details
Build and Test (windows-latest)	129301193821	PASS	Windows job
Build and Test (ubuntu-22.04)	129301194153	PASS	Ubuntu job


Both jobs completed the setup, checkout, CMake configuration, build and test steps successfully.
3.2 Later main-branch CI evidence
A later successful main workflow run was also observed:
- Branch: main
- Commit SHA: 3c53f82927bfa7f81c8aea588116238a6d7b7e88
- Run ID: 43625788622
- Date: 14 November 2026, 18:17:42 UTC
- Result: SUCCESS
- Main-branch workflow run
This is evidence of a successful workflow on main; it is not, by itself, proof that every outstanding feature or every required pull request was integrated and verified.
3.3 Previously mentioned failed run
An earlier version of this report mentioned one failed run among the ten most recent runs inspected at that time. The current GitHub CLI listing of the latest ten runs returned successful runs only. The failed run's ID and commit could not be identified from the available output. Therefore, this report does not claim that the historical failure was resolved by a specific change. The successful run and job IDs above are the verifiable evidence recorded here.
4. Console usability testing
Manual checks were performed for the Administrator, Teacher and Student account flows.
Test	Observed result	Status
Initial administrator setup	Administrator account created successfully	PASS
Administrator login	Administrator menu displayed	PASS
Student creation	Student record created after correcting the contact number to the required 10 digits	PASS
Teacher account creation and login	Teacher account created; Teacher menu displayed	PASS
Student account creation and login	Student account created and linked to roll number 213; Student menu displayed	PASS
Student logout	Returned to the login prompt	PASS
Teacher logout	Returned to the login prompt	PASS


Result: PARTIAL PASS. The role-specific menus and login/logout flows were manually exercised. This record does not claim that every individual report, marks-entry or attendance action was fully tested, nor that the three-minute completion target was timed.
5. Security testing
The inspected implementation and automated tests provide evidence for:
- PBKDF2-HMAC-SHA256 password hashing.
- Login lockout handling.
- Session expiry support.
- Role-based access control.
- Audit logging.
- Input validation.
The related authentication, RBAC, audit and input-validation test suites passed in the recorded regression run.

Result: PARTIAL — NOT A COMPLETE SECURITY TEST CAMPAIGN. Integrated verification of directory permissions and backup/restore behavior remains outstanding in the evidence available for this report. No claim of a complete penetration test or exhaustive security assessment is made.
6. Performance testing
No measured benchmark results against explicit project performance targets are available in the reviewed evidence.

Result: NOT VERIFIED.

To close this item, record the approved target, test workload, environment, measured result and pass/fail decision for each performance requirement. Merely running a benchmark without those measurements and targets is insufficient.
7. Reliability testing
The crash_safe_saving_tests suite passed in the recorded nine-suite regression run.

Result: PARTIAL PASS. This is evidence for the registered crash-safe saving tests only; it does not establish that every backup, restore or recovery scenario has been integrated and manually verified.
8. Integration status and outstanding work
The feature-branch workflow passed on Windows and Ubuntu 22.04 for commit ee16c6a96747f9fcc35bc1a8fc4e7f9b5ec65569. A later workflow on main also passed on 14 November 2026.

The available records do not establish that all required pull requests have been merged, that every feature has been included in the same integrated revision, or that the complete regression suite was run against the final integrated main commit.

Outstanding items:
1. Confirm all required pull requests are merged.
2. Verify backup/restore and directory-permission behavior on the integrated branch.
3. Complete security-specific testing and record its evidence.
4. Execute performance benchmarks against approved measurable targets.
5. Record detailed teacher and student report, marks and attendance action results.
6. Complete the approved SRM-SR-001 through SRM-SR-007 traceability mappings.
7. Run and record final regression tests on the integrated main commit.
8. Confirm final CI status after integration.
9. Conclusion
The recorded feature-branch build and nine registered CTest suites passed. GitHub Actions run 42657880709 independently confirms successful Windows and Ubuntu 22.04 build-and-test jobs on commit ee16c6a96747f9fcc35bc1a8fc4e7f9b5ec65569. A later main workflow, run 43625788622, also succeeded.

The project is not declared fully frozen by this report. Complete system-requirement traceability, measurable performance evidence, full security verification and final integrated-branch regression remain outstanding.
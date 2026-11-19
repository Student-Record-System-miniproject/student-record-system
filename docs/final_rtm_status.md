Interim Requirements Traceability Matrix (RTM) Status Report
Project: Student Record Management System
Report date: 16 November 2026
Status: INTERIM — not the completed final RTM
1. Purpose and status
This document records the traceability evidence currently available for the Student Record Management System. It is intentionally labelled Interim because the approved descriptions and test-case mappings for system requirements SRM-SR-001 through SRM-SR-007 have not been supplied in the project evidence reviewed for this report.

The seven system-requirement rows below are therefore marked BLOCKED. Their descriptions and mappings must be taken from the approved requirements source; they must not be inferred or invented.
2. Status legend
- PASS: Supported by completed test evidence.
- PARTIAL: Some verification exists, but coverage is incomplete.
- NOT VERIFIED: Sufficient test evidence is unavailable.
- BLOCKED: Depends on missing approved requirements, integration, or other outstanding evidence.
3. Requirement-area coverage
Requirement area	Available evidence	Status
Authentication	auth_service_tests; administrator and student login exercised manually	PARTIAL
Student management	student_service_tests; administrator student-management flow exercised	PASS
Grades and reports	grade_service_tests, report_export_tests; report menu availability observed, but individual report results are not recorded here	PARTIAL
Attendance	attendance_service_tests; teacher and student menus exposed attendance options, but individual attendance results are not recorded here	PARTIAL
Administration and access control	rbac_tests, audit_service_tests; administrator menu and user-management flow	PARTIAL
Reliability	crash_safe_saving_tests	PARTIAL
Input validation	input_validator_tests; invalid contact-number input rejected during manual testing	PASS
Usability	Administrator, Teacher and Student login/menu/logout flows manually exercised; individual actions and three-minute target not fully measured	PARTIAL
Portability	Successful Windows and Ubuntu 22.04 jobs in GitHub Actions run 42657880709	PASS
Security	Authentication, RBAC, audit and validation evidence; complete security campaign and integrated directory-permission/backup verification not established	PARTIAL
Performance	No benchmark measurements against project targets available	NOT VERIFIED
Full integration	Feature-branch CI evidence exists; final integrated-branch regression and complete PR integration are not established by this report	BLOCKED


4. System requirements SRM-SR-001 through SRM-SR-007
Mapping status: BLOCKED — approved requirement descriptions and test-case IDs are required.
Requirement ID	Approved description	Test-case ID / evidence	Status
SRM-SR-001	Pending approved requirements source	Pending approved test mapping	BLOCKED
SRM-SR-002	Pending approved requirements source	Pending approved test mapping	BLOCKED
SRM-SR-003	Pending approved requirements source	Pending approved test mapping	BLOCKED
SRM-SR-004	Pending approved requirements source	Pending approved test mapping	BLOCKED
SRM-SR-005	Pending approved requirements source	Pending approved test mapping	BLOCKED
SRM-SR-006	Pending approved requirements source	Pending approved test mapping	BLOCKED
SRM-SR-007	Pending approved requirements source	Pending approved test mapping	BLOCKED


These entries must be replaced with the exact approved requirement descriptions and corresponding test-case IDs/evidence before this document can be called a final RTM.
5. Verification and integration checklist
- [x] Feature-branch CMake configuration and build recorded as successful.
- [x] Nine registered CTest suites recorded as passing.
- [x] GitHub Actions run 42657880709 passed on Windows and Ubuntu 22.04 for commit ee16c6a96747f9fcc35bc1a8fc4e7f9b5ec65569.
- [x] Administrator, Teacher and Student login/menu/logout flows manually exercised.
- [ ] Approved descriptions and test mappings completed for SRM-SR-001 through SRM-SR-007.
- [ ] Detailed manual verification of each teacher/student report, marks and attendance action recorded.
- [ ] Complete security testing and integrated backup/directory-permission verification documented.
- [ ] Performance benchmarks measured against approved project targets.
- [ ] All required pull requests confirmed merged.
- [ ] Final regression suite executed on the integrated main branch and its commit recorded.
- [ ] Final CI run confirmed after integration.
6. Overall status
INTERIM — NOT READY TO BE TREATED AS THE FINAL RTM.

The available evidence supports several functional test areas and successful Windows/Ubuntu CI on the documented feature-branch commit. The seven system-level traceability mappings, performance evidence, complete security verification and final integrated-branch regression remain outstanding.
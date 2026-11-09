# Final Requirements Traceability Matrix (RTM)

**Project:** Student Record Management System  
**Date:** 9 November 2026

## Status legend
- PASS: Supported by completed test evidence.
- PARTIAL: Some verification exists, but coverage is incomplete.
- NOT VERIFIED: Sufficient test evidence is unavailable.
- BLOCKED: Depends on integration or missing requirement information.

## Requirement coverage

| Requirement area | Evidence | Status |
|---|---|---|
| Authentication | auth_service_tests | PARTIAL |
| Student management | student_service_tests | PASS |
| Grades and reports | grade_service_tests, report_export_tests | PASS |
| Attendance | attendance_service_tests | PASS |
| Administration and access control | rbac_tests, audit_service_tests, administrator manual tests | PARTIAL |
| Reliability | crash_safe_saving_tests | PARTIAL |
| Input validation | input_validator_tests | PASS |
| Usability | Administrator console manual test report | PARTIAL |
| Portability | Latest inspected Windows and Ubuntu CI run | PASS |
| Security | Authentication, RBAC, audit and validation evidence | PARTIAL |
| Performance | No dedicated benchmark evidence identified | NOT VERIFIED |
| Full integration | Feature branch verified; final PR merges pending | BLOCKED |

## System Requirements SRM-SR-001 through SRM-SR-007

Descriptions and test-case mappings must be copied from the approved requirements document.

| Requirement ID | Description | Test case mapping | Status |
|---|---|---|---|
| SRM-SR-001 | To be copied from approved requirements | To be mapped | BLOCKED |
| SRM-SR-002 | To be copied from approved requirements | To be mapped | BLOCKED |
| SRM-SR-003 | To be copied from approved requirements | To be mapped | BLOCKED |
| SRM-SR-004 | To be copied from approved requirements | To be mapped | BLOCKED |
| SRM-SR-005 | To be copied from approved requirements | To be mapped | BLOCKED |
| SRM-SR-006 | To be copied from approved requirements | To be mapped | BLOCKED |
| SRM-SR-007 | To be copied from approved requirements | To be mapped | BLOCKED |

## Finalization checklist

- [x] Local build successful.
- [x] Nine registered CTest suites passed.
- [x] Latest inspected CI run successful.
- [x] Windows and Ubuntu CI matrix configured.
- [x] Administrator console usability evidence documented.
- [ ] Teacher and student workflows manually verified.
- [ ] Complete security testing documented.
- [ ] Performance results documented.
- [ ] SRM-SR-001 through SRM-SR-007 mapped to approved test cases.
- [ ] All required PRs merged.
- [ ] Final regression suite rerun on integrated main.
- [ ] Final CI confirmed after integration.

**Overall status: INCOMPLETE — final traceability and integration evidence remain outstanding.**

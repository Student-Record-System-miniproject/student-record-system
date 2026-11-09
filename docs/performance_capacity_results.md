\# Performance and Data Capacity Verification Report



\*\*Issue:\*\* #27  

\*\*Requirements:\*\* SRM-NF-001, SRM-NF-002, SRM-NF-007  

\*\*Test Reference:\*\* TC-Perf-01  

\*\*Status:\*\* Performance and capacity tests executed



\## 1. Test Environment



\- Operating system: Windows

\- Compiler: WinLibs MinGW-w64 (POSIX/UCRT)

\- Language standard: C++17

\- Configuration: Release

\- Benchmark optimization: `-O2`

\- Test data: Synthetic records generated in a temporary directory

\- Actual application data: Not modified by the benchmark



\## 2. Test Methodology



A dedicated benchmark executable, `performance\_capacity\_tests`, was created to verify performance and data capacity requirements.



The benchmark generates:

\- 10,000 student records

\- 50 user accounts

\- 100,000 grade entries

\- 100,000 attendance entries



It performs 20 searches by roll number and 20 searches by name, measuring the average execution time for each operation.



Two startup measurements are performed:



1\. Service initialization: measures construction of AuditService, AuthService and StudentService.

2\. Full application startup: launches the actual `student\_record\_system.exe` using the synthetic application data directory and measures the time from process creation until the `Username:` login prompt appears.



All synthetic files are stored in a temporary directory and removed after the benchmark completes.



\## 3. Performance Results



\### 3.1 Search Response Time (SRM-NF-001)



Requirement: Average search response time must not exceed 2 seconds across 20 searches with 10,000 students.



| Search operation | Average response time | Limit | Result |

|---|---:|---:|---|

| Search by roll number | 0.074235 ms | 2000 ms | PASS |

| Search by name | 1.22096 ms | 2000 ms | PASS |



Both search operations completed comfortably within the required response time.



\### 3.2 Startup Time (SRM-NF-002)



Requirement: Application startup must complete within 5 seconds with 10,000 students.



| Measurement | Result | Limit | Status |

|---|---:|---:|---|

| Service initialization | 17.356 ms | 5000 ms | PASS |

| Full process launch to Username prompt | 161.127 ms | 5000 ms | PASS |



The full startup measurement includes:

\- Launching the actual application executable.

\- Initializing the audit service.

\- Loading 50 user accounts.

\- Loading 10,000 student records.

\- Reaching the interactive login prompt.



The application was launched with its working directory set to the temporary synthetic data directory, leaving actual project data untouched.



\### 3.3 Data Capacity (SRM-NF-007)



Requirement: The system must support at least 10,000 students, 50 user accounts, and 100,000 grade and attendance entries.



| Data type | Required capacity | Successfully loaded | Result |

|---|---:|---:|---|

| Students | 10,000 | 10,000 | PASS |

| User accounts | 50 | 50 | PASS |

| Grade entries | 100,000 | 100,000 | PASS |

| Attendance entries | 100,000 | 100,000 | PASS |



All required record counts were successfully loaded from the generated data files.



Additional loading measurements:



| Data type | Loading time |

|---|---:|

| Grade entries | 142.149 ms |

| Attendance entries | 69.3807 ms |



\## 4. Grade Loading Optimization



Initial benchmarking identified a performance bottleneck in `GradeRepository::load()`.



The original implementation called `upsert()` for every loaded grade record. Since `upsert()` performed a linear search through existing records, loading a large number of records resulted in repeated scans.



The implementation was optimized using an `unordered\_map` to track the position of each unique student-subject-semester combination.



The optimization preserves existing behaviour: duplicate combinations are updated with the latest marks instead of creating duplicate records.



\### Before and After



| Measurement | Before optimization | After optimization |

|---|---:|---:|

| Records loaded | 100,000 | 100,000 |

| Loading time | 33,150 ms | 142.149 ms |



The optimized implementation was approximately 233.2 times faster, reducing grade loading time by approximately 99.57%.



A regression test was added to verify that:

\- All unique records are retained.

\- Duplicate combinations do not create additional records.

\- The latest marks value is preserved.



\## 5. Test Execution



The complete automated test suite was executed in both configurations.



| Configuration | Test suites passed | Result |

|---|---:|---|

| Default/Debug build | 9/9 | PASS |

| Release build | 9/9 | PASS |



The performance and capacity benchmark was executed separately using the Release build.



\## 6. Final Acceptance Criteria



| Acceptance criterion | Status |

|---|---|

| Average roll-number search below 2 seconds | PASS |

| Average name search below 2 seconds | PASS |

| Full application startup below 5 seconds | PASS |

| Support 10,000 student records | PASS |

| Support 50 user accounts | PASS |

| Support 100,000 grade entries | PASS |

| Support 100,000 attendance entries | PASS |

| Grade-loading regression tests pass | PASS |



\## 7. Conclusion



The performance and capacity benchmark demonstrated that the Student Record Management System meets the tested performance and data-capacity requirements.



The full application reached the login prompt in approximately 161 milliseconds with 10,000 student records and 50 user accounts.



Both roll-number and name searches completed well within the two-second average response-time requirement.



The grade-loading bottleneck was identified and optimized, improving loading time from 33.15 seconds to approximately 142 milliseconds for 100,000 records.



All nine automated test suites passed in both the default and Release configurations.



The benchmark results demonstrate that the tested requirements SRM-NF-001, SRM-NF-002 and SRM-NF-007 are satisfied under the documented test environment.


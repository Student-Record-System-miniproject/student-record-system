SRM-NF-004: Console Usability Test Report
Test Reference
TC-Use-01
Objective
Verify that the console menus are understandable, role-specific, and support login, navigation, and logout without unexpectedly terminating the application.
Automated Test Results
Command:
ctest --test-dir build -C Debug --output-on-failure
Result: 9/9 automated tests passed.
Manual Test Results
Test	Expected Result	Observed Result	Status
Initial administrator setup	First administrator account can be created	Account creation completed successfully	PASS
Administrator login	Valid credentials allow login	Administrator menu displayed	PASS
Add student	Valid student details are accepted	Student record creation was completed after correcting the contact number to 10 digits	PASS
Create teacher account	Administrator can create a teacher account	Teacher account was created	PASS
Teacher login	Valid teacher credentials allow login	Teacher menu displayed	PASS
Create student account	Administrator can create a student account linked to a student roll number	Student account was created and linked to roll number 213	PASS
Student login	Valid student credentials allow login	Student menu displayed	PASS
Student logout	Logout returns the user to the login flow	Login prompt was displayed again	PASS
Teacher logout	Logout returns the user to the login flow	Login prompt was displayed again	PASS


Observations and Limitations
- The initial data files were empty when testing began.
- A student contact number must contain exactly 10 digits; the first add attempt was rejected with a validation message, and the record was subsequently created with a valid number.
- The student test account was linked to roll number 213.
- Teacher and Student login/menu/logout flows were manually exercised.
- Detailed functional verification of every report, attendance, and marks operation was not recorded as part of this test run.
- The marks-entry flow requires a configured subject catalogue; the current project does not provide one. The application therefore reports that marks entry is unavailable rather than accepting an unverified subject.
Conclusion
The application builds successfully, all 9 automated tests pass, and the manually exercised Administrator, Teacher, and Student account flows support account creation, login, role-specific menu display, and logout. Further manual verification is still needed for individual report and attendance actions, and marks entry remains dependent on a subject catalogue.
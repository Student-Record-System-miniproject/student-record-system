# SRM-NF-004: Console Usability Test Report

## Test Reference
TC-Use-01

## Objective
Verify that common console tasks can be completed using on-screen menus and that errors explain the cause and corrective action.

## Manual Test Results

| Test | Expected Result | Actual Result | Status |
|---|---|---|---|
| Initial administrator setup | First admin account can be created | Account created successfully | PASS |
| Administrator login | Valid credentials allow login | Login successful | PASS |
| View student records | Student records are displayed | Empty-state message displayed with guidance | PASS |
| Invalid numeric choice (9) | Explain valid menu range | Range 1-3 displayed | PASS |
| Non-numeric choice (abc) | Explain required input | Whole-number guidance displayed | PASS |
| Logout (3) | Return to login | Logout successful | PASS |
| Exit (0) | Close application | Application exited successfully | PASS |
| Login retry | Allow retry after invalid credentials | Retry prompt displayed | PASS |

## Automated Test Results

Command:
`ctest --test-dir build -C Debug --output-on-failure`

Result: 9/9 tests passed.

## Observations and Limitations

- The user database was initially empty. Initial administrator setup was verified.
- The student database was empty, so the empty-record response was tested.
- Administrator console flows were manually tested.
- Teacher and Student role menus were not manually tested.
- The three-minute completion target was not independently timed.

## Conclusion

The tested Administrator console flows provide clear prompts, actionable input-error messages, login retry, logout, and application exit. Automated regression tests passed.

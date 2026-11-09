cmake_minimum_required(VERSION 3.16)
project(student_record_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(srm_core
    src/common_types.cpp
    src/crypto.cpp
    src/audit_service.cpp
    src/storage_utils.cpp
    src/grade_service.cpp
    src/attendance_service.cpp
    src/report_export.cpp
    src/rbac.cpp
    src/student_service.cpp
    src/auth_service.cpp
    src/input_validator.cpp
    src/backup_service.cpp
)

add_library(srm_grade ALIAS srm_core)

target_include_directories(srm_core PUBLIC include)

if (MSVC)
    target_compile_options(srm_core PRIVATE /W4)
else()
    target_compile_options(srm_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(student_record_system
    src/main.cpp
)

target_link_libraries(student_record_system
    PRIVATE srm_core
)

add_executable(grade_service_tests
    tests/grade_service_tests.cpp
)
target_link_libraries(grade_service_tests PRIVATE srm_core)

add_executable(attendance_service_tests
    tests/attendance_service_tests.cpp
)
target_link_libraries(attendance_service_tests PRIVATE srm_core)

add_executable(report_export_tests
    tests/report_export_tests.cpp
)
target_link_libraries(report_export_tests PRIVATE srm_core)

add_executable(rbac_tests
    tests/rbac_tests.cpp
)
target_link_libraries(rbac_tests PRIVATE srm_core)

add_executable(student_service_tests
    tests/student_service_tests.cpp
)
target_link_libraries(student_service_tests PRIVATE srm_core)

add_executable(auth_service_tests
    tests/auth_service_tests.cpp
)
target_link_libraries(auth_service_tests PRIVATE srm_core)

add_executable(audit_service_tests
    tests/audit_service_tests.cpp
)
target_link_libraries(audit_service_tests PRIVATE srm_core)

add_executable(crash_safe_saving_tests
    tests/crash_safe_saving_tests.cpp
)
target_link_libraries(crash_safe_saving_tests PRIVATE srm_core)

add_executable(backup_service_tests
    tests/backup_service_tests.cpp
)
target_link_libraries(backup_service_tests PRIVATE srm_core)
enable_testing()

add_test(NAME grade_service_tests COMMAND grade_service_tests)
add_test(NAME attendance_service_tests COMMAND attendance_service_tests)
add_test(NAME report_export_tests COMMAND report_export_tests)
add_test(NAME rbac_tests COMMAND rbac_tests)
add_test(NAME student_service_tests COMMAND student_service_tests)
add_test(NAME auth_service_tests COMMAND auth_service_tests)
add_test(NAME audit_service_tests COMMAND audit_service_tests)
add_test(NAME crash_safe_saving_tests COMMAND crash_safe_saving_tests)
add_test(NAME input_validator_tests COMMAND input_validator_tests)
add_test(NAME backup_service_tests COMMAND backup_service_tests)

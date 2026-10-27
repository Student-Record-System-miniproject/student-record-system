cmake_minimum_required(VERSION 3.16)
project(student_record_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(srm_grade
    src/grade_service.cpp
    src/attendance_service.cpp
    src/report_export.cpp
    src/rbac.cpp
)
target_include_directories(srm_grade PUBLIC include)

if (MSVC)
    target_compile_options(srm_grade PRIVATE /W4)
else()
    target_compile_options(srm_grade PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(grade_service_tests tests/grade_service_tests.cpp)
target_link_libraries(grade_service_tests PRIVATE srm_grade)

add_executable(attendance_service_tests tests/attendance_service_tests.cpp)
target_link_libraries(attendance_service_tests PRIVATE srm_grade)

add_executable(report_export_tests tests/report_export_tests.cpp)
target_link_libraries(report_export_tests PRIVATE srm_grade)

add_executable(rbac_tests tests/rbac_tests.cpp)
target_link_libraries(rbac_tests PRIVATE srm_grade)

enable_testing()
add_test(NAME grade_service_tests COMMAND grade_service_tests)
add_test(NAME attendance_service_tests COMMAND attendance_service_tests)
add_test(NAME report_export_tests COMMAND report_export_tests)
add_test(NAME rbac_tests COMMAND rbac_tests)

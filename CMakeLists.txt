cmake_minimum_required(VERSION 3.16)
project(student_record_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(srm_grade src/grade_service.cpp)
target_include_directories(srm_grade PUBLIC include)

if (MSVC)
    target_compile_options(srm_grade PRIVATE /W4)
else()
    target_compile_options(srm_grade PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(grade_service_tests tests/grade_service_tests.cpp)
target_link_libraries(grade_service_tests PRIVATE srm_grade)

enable_testing()
add_test(NAME grade_service_tests COMMAND grade_service_tests)
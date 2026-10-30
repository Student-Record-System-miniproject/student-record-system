cmake_minimum_required(VERSION 3.15)

project(StudentRecordSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_library(srm_core
    src/file_repository.cpp
    src/auth_manager.cpp
)

add_executable(student_record_system
    src/main.cpp
)

target_link_libraries(student_record_system
    PRIVATE srm_core
)

enable_testing()

add_executable(file_repository_tests
    tests/file_repository_tests.cpp
)

target_link_libraries(file_repository_tests
    PRIVATE srm_core
)

add_test(
    NAME FileRepositoryTests
    COMMAND file_repository_tests
)

add_executable(auth_manager_tests
    tests/auth_manager_tests.cpp
)

target_link_libraries(auth_manager_tests
    PRIVATE srm_core
)

add_test(
    NAME AuthManagerTests
    COMMAND auth_manager_tests
)
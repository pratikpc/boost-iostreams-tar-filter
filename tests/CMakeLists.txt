cmake_minimum_required(VERSION 3.14)

# Add test executable
add_executable(${PROJECT_NAME}_tests test_boost_iostreams_tar_filter.cxx)

find_package(GTest CONFIG REQUIRED)

# Link your INTERFACE library and GTest
target_link_libraries(
    ${PROJECT_NAME}_tests
    PRIVATE
        ${TARGET_NAME} # your interface library
        GTest::gtest
        GTest::gtest_main
)

# Include headers
target_include_directories(
    ${PROJECT_NAME}_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

find_path(PICOSHA2_INCLUDE_DIRS "picosha2.h")
target_include_directories(
    ${PROJECT_NAME}_tests
    PRIVATE
        ${PICOSHA2_INCLUDE_DIRS}
)

# Use gtest_discover_tests to auto-register tests with CTest
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)

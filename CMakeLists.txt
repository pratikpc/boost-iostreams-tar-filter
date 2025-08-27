cmake_minimum_required(VERSION 3.14)
project(boost-iostreams-tar-filter LANGUAGES CXX VERSION 1.0.0)

# Sanitized target name (dash `-` not allowed)
set(TARGET_NAME boost_iostreams_tar_filter)

# Create STATIC library target
add_library(${TARGET_NAME} STATIC)

# Link dependencies
find_package(Boost REQUIRED COMPONENTS iostreams)

# Include directories (public)
target_include_directories(
    ${TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Include directories (private)
target_include_directories(
    ${TARGET_NAME}
    PRIVATE
        include-private
)

target_link_libraries(${TARGET_NAME} PUBLIC Boost::iostreams)

target_sources(${TARGET_NAME} PRIVATE src/base-tar-filter-impl.cxx)

option(BOOST_IOSTREAMS_TAR_FILTER_BUILD_TESTING "Enable testing" OFF)
if(BOOST_IOSTREAMS_TAR_FILTER_BUILD_TESTING)
    # Enable CTest support
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif(BOOST_IOSTREAMS_TAR_FILTER_BUILD_TESTING)

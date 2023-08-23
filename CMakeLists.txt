cmake_minimum_required(VERSION 3.24)

project(qc-image)

# Fetch dependencies
if(${PROJECT_IS_TOP_LEVEL})
    include(FetchContent)

    FetchContent_Declare(qc-cmake SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qc-cmake)

    FetchContent_Declare(qc-core SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qc-core OVERRIDE_FIND_PACKAGE)

    option(BUILD_GMOCK "" OFF)
    option(INSTALL_GTEST "" OFF)
    option(gtest_force_shared_crt "" ON)
    FetchContent_Declare(GTest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0 OVERRIDE_FIND_PACKAGE)

    FetchContent_MakeAvailable(GTest qc-cmake qc-core)
endif()

find_package(qc-core)

qc_setup_target(
    qc-image
    STATIC_LIBRARY
    PUBLIC_LINKS
        qc-core
)

if(${PROJECT_IS_TOP_LEVEL})
    add_subdirectory(test EXCLUDE_FROM_ALL)
endif()

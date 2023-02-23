cmake_minimum_required(VERSION 3.23)

project(qc-image)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

find_package(qc-core)

qc_setup_target(
    qc-image
    STATIC_LIBRARY
    PUBLIC_LINKS
        qc-core::qc-core
)

add_subdirectory(test)

qc_setup_install(
    TARGETS
        qc-image
    DEPENDENCIES
        qc-core
)

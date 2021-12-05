cmake_minimum_required(VERSION 3.20)

project(qc-image)

set(CMAKE_CXX_STANDARD 20)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

qc_setup_general()

### DEPENDENCIES ###############################################################

find_package(qc-core REQUIRED)

### QC-IMAGE ###################################################################

qc_setup_target(
    qc-image
    STATIC_LIBRARY
    PUBLIC_LINKS
        qc-core::qc-core
)

### SUBDIRECTORIES #############################################################

add_subdirectory(test EXCLUDE_FROM_ALL)

### INSTALL ####################################################################

qc_setup_install(
    TARGETS
        qc-image
    DEPENDENCIES
        qc-core
)

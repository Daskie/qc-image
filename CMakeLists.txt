cmake_minimum_required(VERSION 3.17)

project(qc-image)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX -d)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

### DEPENDENCIES ###############################################################

find_package(qc-core REQUIRED)

### QC-IMAGE ###################################################################

file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_library(qc-image STATIC ${SRC_FILES})

target_include_directories(
    qc-image
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        external
)

target_link_libraries(
    qc-image
    PUBLIC
        qc-core::qc-core
)

target_compile_options(qc-image PRIVATE ${QC_WARNINGS_ERROR})

### SUBDIRECTORIES #############################################################

add_subdirectory(test)

### INSTALL ####################################################################

qc_install(DEPENDENCIES qc-core)

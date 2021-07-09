cmake_minimum_required(VERSION 3.17)

project(qc-image)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX -d)

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

if(MSVC)
    target_compile_options(qc-image PRIVATE /W4 /WX)
else()
    message(FATAL_ERROR "TODO")
endif()

### SUBDIRECTORIES #############################################################

add_subdirectory(test)

### INSTALL ####################################################################

# See https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html

include(GNUInstallDirs)

# General install and target setup
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include DESTINATION .)

# Export targets to create importable cmake file
install(
    EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${CMAKE_PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Allow library to be found via find_package
include(CMakePackageConfigHelpers)

# Create config.cmake files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Install config.cmake files
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

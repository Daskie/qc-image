### QC-IMAGE-TEST ##############################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-image-test ${SRC_FILES})

target_link_libraries(qc-image-test PRIVATE qc-image)

target_compile_options(qc-image-test PRIVATE ${QC_WARNINGS_ERROR})

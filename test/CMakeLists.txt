### QC-IMAGE-TEST ##############################################################

qc_setup_target(
    qc-image-test
    EXECUTABLE
    SOURCE_FILES
        *.cpp
    PRIVATE_LINKS
        qc-image
)

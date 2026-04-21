# CopyIfDifferent.cmake
# Usage: cmake -DSOURCE=src_dir -DDEST=dst_dir -P CopyIfDifferent.cmake

if(NOT SOURCE OR NOT DEST)
    message(FATAL_ERROR "SOURCE and DEST must be defined")
endif()

# file(COPY ...) is smart: it only copies files if they are newer or have different size
file(COPY "${SOURCE}/" DESTINATION "${DEST}")

# CopyIfDifferent.cmake
# Usage: cmake -DSOURCE=src_dir -DDEST=dst_dir -P CopyIfDifferent.cmake

if(NOT SOURCE OR NOT DEST)
    message(FATAL_ERROR "SOURCE and DEST must be defined")
endif()

# Gather all files from SOURCE recursively and copy each one individually.
# This way a single locked/permission-denied file does not abort the whole sync.
file(GLOB_RECURSE _all_files RELATIVE "${SOURCE}" "${SOURCE}/*")
foreach(_rel IN LISTS _all_files)
    set(_src "${SOURCE}/${_rel}")
    set(_dst "${DEST}/${_rel}")
    # Only copy when the destination is missing or older than the source
    if(NOT EXISTS "${_dst}" OR "${_src}" IS_NEWER_THAN "${_dst}")
        get_filename_component(_dst_dir "${_dst}" DIRECTORY)
        file(MAKE_DIRECTORY "${_dst_dir}")
        file(COPY_FILE "${_src}" "${_dst}" ONLY_IF_DIFFERENT RESULT _copy_result)
        if(NOT _copy_result EQUAL 0)
            message(WARNING "CopyIfDifferent: Could not copy '${_rel}' (${_copy_result}) — file may be in use, skipping.")
        endif()
    endif()
endforeach()

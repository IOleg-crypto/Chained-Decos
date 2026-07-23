# pack dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/pack/CMakeLists.txt")
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)

    # Guard: if zstd is already built (from zstd.cmake), prevent pack from
    # rebuilding its own copy. The target name "libzstd_static" is shared.
    if(TARGET libzstd_static)
        set(_PACK_SKIP_ZSTD TRUE)
    else()
        set(_PACK_SKIP_ZSTD FALSE)
    endif()

    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/pack"
        "${CMAKE_BINARY_DIR}/vendor/pack" EXCLUDE_FROM_ALL)

    unset(_PACK_SKIP_ZSTD)
endif()
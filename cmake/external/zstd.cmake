# Zstandard compression library
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "Build zstd contrib" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static library" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared library" FORCE)
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "Zstd legacy support" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zstd/build/cmake EXCLUDE_FROM_ALL)

# Unity build causes redefinition errors in cover.h (no include guard)
if(TARGET libzstd_static)
    set_target_properties(libzstd_static PROPERTIES UNITY_BUILD OFF)
endif()

# --- Create zstd:: namespace aliases for downstream consumers (pak_archive) ---
# This runs immediately after zstd targets are created, so aliases are ready
# before pak_archive.cmake is included.
if(TARGET libzstd_static AND NOT TARGET zstd::libzstd_static)
    add_library(zstd::libzstd_static ALIAS libzstd_static)
endif()
if(TARGET libzstd_shared AND NOT TARGET zstd::libzstd_shared)
    add_library(zstd::libzstd_shared ALIAS libzstd_shared)
endif()

set(ZSTD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zstd/lib CACHE PATH "Zstd include directory" FORCE)
set(ZSTD_LIBRARY libzstd_static CACHE STRING "Zstd library target" FORCE)

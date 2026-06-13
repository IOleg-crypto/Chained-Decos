# Zstandard compression library
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Build zstd programs" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "Build zstd tests" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "Build zstd contrib" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Build zstd static library" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Build zstd shared library" FORCE)
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "Zstd legacy support" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/zstd/build/cmake EXCLUDE_FROM_ALL)

set(ZSTD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/zstd/lib CACHE PATH "Zstd include directory" FORCE)
set(ZSTD_LIBRARY libzstd_static CACHE STRING "Zstd library target" FORCE)

if(TARGET libzstd_static)
endif()

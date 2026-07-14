cmake_minimum_required(VERSION 3.8)
project(gpak_externals)

add_library(${PROJECT_NAME} STATIC gpakext.cpp)

if(WIN32)
add_subdirectory(libfmemopen)
target_link_libraries(${PROJECT_NAME} PUBLIC libfmemopen)
endif()

# ZLIB: Use assimp's vendored zlib (target 'zlibstatic') via ZLIB::ZLIB alias
# created in pak_archive.cmake before this file is included.
# zstd: Use root-level zstd (built via cmake/external/zstd.cmake) with zstd:: namespace aliases.
target_link_libraries(${PROJECT_NAME}
	PUBLIC ZLIB::ZLIB
	PRIVATE $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)

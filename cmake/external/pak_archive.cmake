# GPAK (pak_archive) game archive library — https://github.com/AdamFull/pak_archive
set(BUILD_SHARED OFF CACHE BOOL "Build gpak as a shared library" FORCE)

# gpak's deflate compressor needs ZLIB. We don't vendor zlib directly, but assimp
# ships a copy under contrib/zlib and (with no system zlib found) builds it as
# `zlibstatic` — reuse that target instead of requiring a system/vcpkg ZLIB.
# This module must be include()'d after assimp so the target already exists.
if(NOT TARGET zlibstatic)
    message(FATAL_ERROR "pak_archive.cmake expects assimp's vendored zlib (target "
        "'zlibstatic') to already exist — make sure include(assimp) runs before "
        "include(pak_archive) in Dependencies.cmake.")
endif()

# zlibstatic sets its include dirs via the legacy include_directories() rather than
# target_include_directories(), so nothing propagates through our ZLIB::ZLIB alias
# (defined in pak_archive/external/CMakeLists.txt) — add them explicitly. zconf.h is
# generated into zlibstatic's binary dir, not its source dir, so both are needed.
get_target_property(_zlibstatic_source_dir zlibstatic SOURCE_DIR)
get_target_property(_zlibstatic_binary_dir zlibstatic BINARY_DIR)
target_include_directories(zlibstatic INTERFACE
    $<BUILD_INTERFACE:${_zlibstatic_source_dir}>
    $<BUILD_INTERFACE:${_zlibstatic_binary_dir}>
)
unset(_zlibstatic_source_dir)
unset(_zlibstatic_binary_dir)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pak_archive/lib EXCLUDE_FROM_ALL)

if(TARGET libgpak)
    set_target_properties(libgpak PROPERTIES UNITY_BUILD OFF)

    # libgpak's own sources (gpak_compressors.c) include <zstd.h> directly, but only link
    # gpak_externals PRIVATELY, so gpak_externals' PRIVATE zstd link doesn't reach them —
    # link zstd into libgpak itself too, just for the include dirs it carries.
    target_link_libraries(libgpak PRIVATE
        $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
    )
endif()

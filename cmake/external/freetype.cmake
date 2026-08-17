# FreeType2 — configured as a static library with minimal options
if(TARGET freetype)
    return()
endif()

set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_LIBPNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_UNSYSCALL_HACK ON CACHE BOOL "" FORCE)
# Build as static lib with /MT on MSVC
set(FT_WITH_ZLIB OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/freetype" "${CMAKE_BINARY_DIR}/freetype" EXCLUDE_FROM_ALL)

# freetype-gl — minimal build: only atlas + font (no vertex-buffer / text-buffer / OpenGL)
if(TARGET engine_freetype_gl)
    return()
endif()

set(FREETYPE_GL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/freetype-gl")

add_library(engine_freetype_gl STATIC
    "${FREETYPE_GL_DIR}/texture-atlas.c"
    "${FREETYPE_GL_DIR}/texture-font.c"
    "${FREETYPE_GL_DIR}/vector.c"
    "${FREETYPE_GL_DIR}/utf8-utils.c"
    "${FREETYPE_GL_DIR}/ftgl-utils.c"
    "${FREETYPE_GL_DIR}/distance-field.c"
    "${FREETYPE_GL_DIR}/edtaa3func.c"
)

target_include_directories(engine_freetype_gl PUBLIC "${FREETYPE_GL_DIR}")
target_link_libraries(engine_freetype_gl PUBLIC freetype)

# freetype-gl headers declare ftgl namespace when compiled as C++
# — suppress any warnings from its C code on MSVC
if(MSVC)
    target_compile_options(engine_freetype_gl PRIVATE /W0)
else()
    target_compile_options(engine_freetype_gl PRIVATE -w)
endif()

# freetype-gl's texture-font.c has a broken fallback for MSVC that
# undefines inline and tries to define __builtin_bswap32 — define
# __GNUC__ for clang so it skips that code path.
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(engine_freetype_gl PRIVATE __GNUC__)
endif()

# Coral dependency (Scripting host)
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/coral/cmake/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/coral/cmake" "${CMAKE_BINARY_DIR}/vendor/coral" EXCLUDE_FROM_ALL)

    # Coral.Native must be compiled in strict isolation from the engine's unity/PCH build.
    # MSVC in C++20 mode deletes operator<<(const wchar_t*) on narrow ostreams (strict
    # conformance change). Coral internally uses wchar_t conversion helpers that trip this
    # when compiled inside a merged unity TU that includes the engine's C++20 PCH.
    # Solution: force C++17, disable unity build AND engine PCH carry-over for this target.
    set_target_properties(Coral.Native PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        UNITY_BUILD OFF
    )

    # Suppress the engine's precompiled header from being injected into Coral compilation units.
    # CMake propagates PCH via target_precompile_headers(... REUSE_FROM ...) chains;
    # marking SKIP_PRECOMPILE_HEADERS prevents that on each source file.
    get_target_property(_coral_srcs Coral.Native SOURCES)
    foreach(_src ${_coral_srcs})
        if(_src MATCHES "\\.cpp$")
            set_source_files_properties("${_src}" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
        endif()
    endforeach()

    # MSVC-specific: suppress size-conversion warnings that are expected in Coral
    if(MSVC)
        target_compile_options(Coral.Native PRIVATE /wd4267 /wd4244 /wd4018)
    endif()

    # MinGW fixes for Coral
    if(MINGW)
        set(CORAL_FIX_DIR "${CMAKE_BINARY_DIR}/vendor/coral_fixes")
        if(NOT EXISTS "${CORAL_FIX_DIR}")
            file(MAKE_DIRECTORY "${CORAL_FIX_DIR}")
        endif()
        file(WRITE "${CORAL_FIX_DIR}/ShlObj_core.h" "#pragma once\n#include <shlobj.h>\n")
        target_include_directories(Coral.Native PRIVATE "${CORAL_FIX_DIR}")
    endif()
else()
    message(FATAL_ERROR "Coral submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/coral/cmake")
endif()

# Chained Engine - Compiler Settings
# Extracted from root CMakeLists.txt for modularity

# Compiler-specific settings
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()
if(MSVC)
    # MSVC-specific settings
    add_compile_options(
        $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/MT> $<$<CONFIG:Release>:/DNDEBUG>
        /Zi /EHsc
        /MP                  # Multi-processor compilation
        /Zc:preprocessor     # Modern preprocessor
        /Gm-                 # Disable minimal rebuild (it's slower)
        /utf-8               # Use UTF-8 character set
    )

    if(ENABLE_WARNINGS)
        add_compile_options(/W4 /permissive-)
        if(WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
    else()
        add_compile_options(/W0)
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(/fsanitize=address)
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang settings
    add_compile_options(
        $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-DNDEBUG>
    )

    if(ENABLE_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Wmost -Wno-missing-braces -Wno-missing-field-initializers -Wno-attributes)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-Wno-missing-field-initializers -Wno-attributes)
        endif()

        if(WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    else()
        add_compile_options(-w)
    endif()

    if(ENABLE_DEBUG_INFO)
        add_compile_options(-g)
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()

    if(ENABLE_PROFILING)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()

    if(WIN32)
        # Force static linkage of runtimes for MinGW/GCC to avoid DLL entry point mismatches in CI/Bash environments
        add_link_options(-static -static-libgcc -static-libstdc++)
    endif()
endif()

# Platform-specific settings
if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7+
endif()

# Optimized Build Settings
option(ENABLE_UNITY_BUILD "Enable Unity Builds for faster compilation" ON)
option(ENABLE_PCH "Enable Precompiled Headers for faster compilation" ON)

if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
endif()

# Function to apply common engine optimizations to a target
function(apply_engine_optimizations target_name)
    if(ENABLE_PCH)
        # We use a header file for PCH to handle complex logic like undefining Windows macros
        set(PCH_HEADER_CONTENT "
#include \"engine/core/base.h\"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <raylib.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#ifdef CH_PLATFORM_WINDOWS
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  
  // Temporarily rename Windows functions that conflict with Raylib
  #define ShowCursor _win_ShowCursor
  #define CloseWindow _win_CloseWindow
  #define Rectangle _win_Rectangle
  #define DrawText _win_DrawText
  #define DrawTextEx _win_DrawTextEx
  #define LoadImage _win_LoadImage
  
  #include <windows.h>
  
  // Restore names so Raylib can use them
  #undef ShowCursor
  #undef CloseWindow
  #undef Rectangle
  #undef DrawText
  #undef DrawTextEx
  #undef LoadImage
#endif

#include <raylib.h>
#include <imgui.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
")
        # Create a temp file for PCH
        set(PCH_FILE "${CMAKE_BINARY_DIR}/engine_pch.h")
        file(WRITE "${PCH_FILE}" "${PCH_HEADER_CONTENT}")
        
        target_precompile_headers(${target_name} PUBLIC "${PCH_FILE}")
    endif()
endfunction()

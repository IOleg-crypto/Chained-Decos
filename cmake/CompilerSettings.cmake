# Chained Engine - Compiler Settings
# Extracted from root CMakeLists.txt for modularity

# Compiler-specific settings
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
        /bigobj              # Allow large object files (required for many modules)
        
        # Dead Code Elimination: Function-Level Linking
        $<$<CONFIG:Release>:/Gy>
    )
    
    # Strip unused functions in Release
    add_link_options($<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)

    if(DISABLE_ALL_WARNINGS)
        add_compile_options(/W0)
    elseif(ENABLE_WARNINGS)
        add_compile_options(/W4 /permissive-)
        if(WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
    else()
        add_compile_options(/W1)
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(/fsanitize=address)
    endif()

    # Level 2 Security Hardening
    add_compile_options(/guard:cf /GS)
    add_link_options(/DYNAMICBASE /NXCOMPAT /guard:cf)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang settings
    add_compile_options(
        $<$<CONFIG:Debug>:-O0> $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-DNDEBUG>
        -Wa,-mbig-obj        # Allow large object files (MinGW)
        
        # Dead Code Elimination data sections
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
    )

    # Dead Code Elimination linkage and binary stripping for Release build
    add_link_options(
        $<$<CONFIG:Release>:-Wl,--gc-sections>
        $<$<CONFIG:Release>:-s>
    )

    if(DISABLE_ALL_WARNINGS)
        add_compile_options(-w)
    elseif(ENABLE_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Wmost -Wno-missing-braces -Wno-missing-field-initializers -Wno-attributes)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            add_compile_options(-Wno-missing-field-initializers -Wno-attributes)
        endif()

        if(WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    else()
        add_compile_options(-Wno-all)
    endif()

    if(ENABLE_DEBUG_INFO)
        add_compile_options(-g)
    endif()

    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()

    # (Linux Only)
    if(NOT WIN32)
        add_link_options(-Wl,-z,relro -Wl,-z,now)
    endif()

    if(ENABLE_PROFILING)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()

    if(WIN32)
        # NOTE: Disabled static runtime for DLL support
        # Force static linkage of runtimes for MinGW/GCC to avoid DLL entry point mismatches in CI/Bash environments
        # add_link_options(-static -static-libgcc -static-libstdc++)
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
option(ENABLE_UNITY_BUILD "Enable Unity Builds for faster compilation" OFF)
option(ENABLE_PCH "Enable Precompiled Headers for faster compilation" ON)
option(ENABLE_LTO "Enable Link-Time Optimization (IPO) for Release configurations" ON)
option(ENABLE_COVERAGE "Enable Code Coverage (GCC/Clang only)" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    endif()
endif()

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
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
")
        # Create a temp file for PCH and only copy if changed
        set(PCH_FILE_TMP "${CMAKE_BINARY_DIR}/engine_pch.h.tmp")
        set(PCH_FILE "${CMAKE_BINARY_DIR}/engine_pch.h")
        file(WRITE "${PCH_FILE_TMP}" "${PCH_HEADER_CONTENT}")
        configure_file("${PCH_FILE_TMP}" "${PCH_FILE}" COPYONLY)
        
        target_precompile_headers(${target_name} PUBLIC "${PCH_FILE}")
    endif()

    if(ENABLE_LTO)
        # Disable LTO for MinGW for now as it has issues with PCH/Plugins in this environment
        if(MINGW)
            set(ipo_supported OFF)
        else()
            include(CheckIPOSupported)
            check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
        endif()

        if(ipo_supported)
            set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        elseif(ipo_output)
            message(WARNING "IPO/LTO is not supported by the current compiler: ${ipo_output}")
        endif()
    endif()
endfunction()

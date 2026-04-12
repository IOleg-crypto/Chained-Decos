# Helper function to compile C# scripts for a game project
# Usage: chained_add_csharp_scripts(TARGET_NAME CSHARP_PROJECT_PATH)
function(chained_add_csharp_scripts TARGET_NAME CSHARP_PROJECT_PATH)
    set(SCRIPT_TARGET "BuildScripts_${TARGET_NAME}")
    if(TARGET ${SCRIPT_TARGET})
        return()
    endif()

    set(FULL_CSPROJ_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${CSHARP_PROJECT_PATH}")
    if(NOT EXISTS "${FULL_CSPROJ_PATH}")
        message(WARNING "chained_add_csharp_scripts: Could not find ${FULL_CSPROJ_PATH}")
        return()
    endif()

    set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/include/coral/cmake")
    set(SCRIPT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/scripts/${TARGET_NAME}")
    
    # Track only gameplay .cs files for incremental builds
    file(GLOB_RECURSE CS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cs")
    set(SCRIPT_DLL_PATH "${SCRIPT_OUTPUT_DIR}/${TARGET_NAME}.dll")

    add_custom_command(
        OUTPUT "${SCRIPT_DLL_PATH}"
        COMMAND dotnet build "${FULL_CSPROJ_PATH}"
                -c $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:>>,Debug,Release> 
                --output "${SCRIPT_OUTPUT_DIR}" 
                -p:CoralManagedDir="${CORAL_MANAGED_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        DEPENDS ${FULL_CSPROJ_PATH} ${CS_SOURCES}
        COMMENT "Building C# Scripts for ${TARGET_NAME} (incremental)"
    )

    add_custom_target(${SCRIPT_TARGET} ALL DEPENDS "${SCRIPT_DLL_PATH}")
    
    # Ensure scripts build AFTER CHEngine_Managed to avoid dotnet race condition
    if(TARGET CHEngine_Managed)
        add_dependencies(${SCRIPT_TARGET} CHEngine_Managed)
    endif()
endfunction()

# Internal helper to apply common configuration to game-related targets
macro(_chained_configure_game_target TGT)
    target_include_directories(${TGT} PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}
    )
    apply_engine_optimizations(${TGT})
endmacro()

# Main helper function to create a standalone game project
# Usage: 
# chained_add_game(TARGET_NAME
#    DISPLAY_NAME "Project Title"
#    CSHARP_PROJECT "src/Scripts.csproj" # Optional
#    SOURCES src/file1.cpp src/file2.cpp # Optional
# )
function(chained_add_game TARGET_NAME)
    set(options)
    set(oneValueArgs PROJECT_GAME CSHARP_PROJECT)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(GAME "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 1. Locate the entry point (main.cpp)
    set(ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    if(NOT EXISTS "${ENTRY_SOURCE}")
        message(FATAL_ERROR "chained_add_game: Could not find src/main.cpp for ${TARGET_NAME}")
    endif()

    # 2. Create C++ libraries if native sources are provided
    if(GAME_SOURCES)
        # Static library for logic/tests
        add_library(${TARGET_NAME} STATIC ${GAME_SOURCES})
        target_link_libraries(${TARGET_NAME} PUBLIC engine)
        _chained_configure_game_target(${TARGET_NAME})
        
        # Shared library for Hot Reload
        add_library(${TARGET_NAME}Module SHARED ${GAME_SOURCES})
        target_link_libraries(${TARGET_NAME}Module PUBLIC engine)
        target_compile_definitions(${TARGET_NAME}Module PRIVATE GAME_BUILD_DLL)
        set_target_properties(${TARGET_NAME}Module PROPERTIES OUTPUT_NAME "${TARGET_NAME}")
        _chained_configure_game_target(${TARGET_NAME}Module)
    endif()

    # 3. Create the EXECUTABLE target
    add_executable(${TARGET_NAME}Exe ${ENTRY_SOURCE})
    target_link_libraries(${TARGET_NAME}Exe PRIVATE RuntimeCore)
    
    if(GAME_SOURCES)
         target_link_libraries(${TARGET_NAME}Exe PRIVATE ${TARGET_NAME})
    endif()
    
    set_target_properties(${TARGET_NAME}Exe PROPERTIES 
        OUTPUT_NAME "${TARGET_NAME}"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    
    target_compile_definitions(${TARGET_NAME}Exe PRIVATE 
        GAME_BUILD_EXE
    )
    _chained_configure_game_target(${TARGET_NAME}Exe)

    # 4. Handle C# Script Building
    if(GAME_CSHARP_PROJECT)
        chained_add_csharp_scripts(${TARGET_NAME} "${GAME_CSHARP_PROJECT}")
        add_dependencies(${TARGET_NAME}Exe "BuildScripts_${TARGET_NAME}")
    endif()

    # 5. Installation
    set(INSTALL_TARGETS ${TARGET_NAME}Exe)
    if(TARGET ${TARGET_NAME})
        list(APPEND INSTALL_TARGETS ${TARGET_NAME})
    endif()
    if(TARGET ${TARGET_NAME}Module)
        list(APPEND INSTALL_TARGETS ${TARGET_NAME}Module)
    endif()

    install(TARGETS ${INSTALL_TARGETS}
        RUNTIME DESTINATION bin COMPONENT Runtime
        ARCHIVE DESTINATION lib COMPONENT Runtime
        LIBRARY DESTINATION lib COMPONENT Runtime
    )

    message(STATUS "Configured Project: ${GAME_PROJECT_GAME} (Exe=${TARGET_NAME}Exe, Output=${TARGET_NAME})")
endfunction()

# Generate a header with the build preset names from CMakePresets.json so C++
# code does not need to duplicate or hardcode preset lists.
function(chained_generate_build_preset_header)
    set(PRESETS_FILE "${CMAKE_SOURCE_DIR}/CMakePresets.json")
    if(NOT EXISTS "${PRESETS_FILE}")
        message(FATAL_ERROR "chained_generate_build_preset_header: Could not find ${PRESETS_FILE}")
    endif()

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PRESETS_FILE}")

    file(READ "${PRESETS_FILE}" PRESETS_JSON)

    set(BUILD_PRESET_NAMES "")
    string(JSON BUILD_PRESET_COUNT LENGTH "${PRESETS_JSON}" buildPresets)
    if(BUILD_PRESET_COUNT GREATER 0)
        math(EXPR LAST_INDEX "${BUILD_PRESET_COUNT} - 1")
        foreach(INDEX RANGE 0 ${LAST_INDEX})
            string(JSON CONFIGURE_PRESET_NAME GET "${PRESETS_JSON}" buildPresets ${INDEX} configurePreset)
            if(CONFIGURE_PRESET_NAME STREQUAL "")
                string(JSON CONFIGURE_PRESET_NAME GET "${PRESETS_JSON}" buildPresets ${INDEX} name)
            endif()

            if(NOT CONFIGURE_PRESET_NAME STREQUAL "")
                list(APPEND BUILD_PRESET_NAMES "${CONFIGURE_PRESET_NAME}")
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES BUILD_PRESET_NAMES)
    list(LENGTH BUILD_PRESET_NAMES BUILD_PRESET_COUNT)

    set(BUILD_PRESET_ENTRIES "")
    foreach(PRESET_NAME IN LISTS BUILD_PRESET_NAMES)
        string(APPEND BUILD_PRESET_ENTRIES "    \"${PRESET_NAME}\",\n")
    endforeach()

    set(GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/chaineddecos")
    file(MAKE_DIRECTORY "${GENERATED_INCLUDE_DIR}")

    set(GENERATED_HEADER "${GENERATED_INCLUDE_DIR}/build_preset_names.h")

    set(CH_BUILD_PRESET_NAMES_INCLUDE_DIR "${GENERATED_INCLUDE_DIR}" PARENT_SCOPE)
    set(CH_BUILD_PRESET_NAMES_HEADER "${GENERATED_HEADER}" PARENT_SCOPE)
    set(CH_BUILD_PRESET_NAMES_COUNT "${BUILD_PRESET_COUNT}" PARENT_SCOPE)

    set(GENERATED_HEADER_CONTENT "#pragma once\n\n#include <array>\n\nnamespace CHEngine::detail\n{\ninline constexpr std::array<const char*, ${BUILD_PRESET_COUNT}> kBuildPresetNames = {\n${BUILD_PRESET_ENTRIES}};\n} // namespace CHEngine::detail\n")
    file(GENERATE OUTPUT "${GENERATED_HEADER}" CONTENT "${GENERATED_HEADER_CONTENT}")
endfunction()

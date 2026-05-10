add_library(engine_scripting ${SUBMODULE_LIB_TYPE}
    scriptengine.h
    scriptengine.cpp
    scriptengine_services.h
    scriptengine_services.cpp
    script_glue.h
    script_glue.cpp
    script_glue_internal.h
    script_glue_system.cpp
    script_glue_input.cpp
    script_glue_scene.cpp
    script_glue_entity.cpp
    script_glue_camera.cpp
    script_glue_ui.cpp
    script_glue_audio.cpp
)

target_include_directories(engine_scripting PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CH_BUILD_PRESET_NAMES_INCLUDE_DIR)
    target_include_directories(engine_scripting PRIVATE
        ${CH_BUILD_PRESET_NAMES_INCLUDE_DIR}
    )
endif()

target_link_libraries(engine_scripting 
PUBLIC 
    engine_core
    engine
    engine_audio
    engine_physics
    ChainedEngine::External::YamlCpp
    ChainedEngine::External::EnTT
    ChainedEngine::External::GLM
    ChainedEngine::External::Coral
    ChainedEngine::External::Miniaudio
)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_scripting)
endif()

if(MSVC)
    target_compile_options(engine_scripting PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_scripting PRIVATE -Wa,-mbig-obj)
endif()

# ── C# Managed API Build ──────────────────────────────────────────────────
# This builds the core engine scripting API (.dll) which the game depends on.

find_program(DOTNET_EXECUTABLE dotnet)
if(DOTNET_EXECUTABLE)
    set(MANAGED_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managed/CHEngine.Managed.csproj")
    set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
    set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/vendor/coral")
    
    # Generate ManagedDependencies.props to provide paths to the .csproj
    # Only regenerate if content changes using copy_if_different
    file(RELATIVE_PATH RELATIVE_CORAL_MANAGED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/managed" "${CORAL_MANAGED_DIR}")
    file(TO_NATIVE_PATH "${RELATIVE_CORAL_MANAGED_DIR}" NATIVE_RELATIVE_CORAL_MANAGED_DIR)
    
    set(PROPS_CONTENT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/managed/ManagedDependencies.props")
    set(PROPS_TEMPLATE_CONTENT "<Project>\n  <PropertyGroup>\n    <CoralManagedDir>$(MSBuildThisFileDirectory)${NATIVE_RELATIVE_CORAL_MANAGED_DIR}</CoralManagedDir>\n  </PropertyGroup>\n</Project>")
    
    # Write to temp file and only update if different (prevents unnecessary rebuilds)
    file(WRITE "${CMAKE_BINARY_DIR}/.cmake_temp_props" "${PROPS_TEMPLATE_CONTENT}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/.cmake_temp_props" "${PROPS_CONTENT_FILE}")
    
    # 1. Collect all local managed sources for dependency tracking
    file(GLOB_RECURSE MANAGED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
    list(APPEND MANAGED_SOURCES "${MANAGED_PROJECT_PATH}")
    list(APPEND MANAGED_SOURCES "${PROPS_CONTENT_FILE}")

    set(MANAGED_DLL_OUTPUT "${MANAGED_OUTPUT_DIR}/CHEngine.Managed.dll")

    add_custom_command(
        OUTPUT "${MANAGED_DLL_OUTPUT}"
        COMMAND ${DOTNET_EXECUTABLE} build "${MANAGED_PROJECT_PATH}" -c $<CONFIG> --output "${MANAGED_OUTPUT_DIR}" -p:CoralManagedDir="${CORAL_MANAGED_DIR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/managed"
        DEPENDS ${MANAGED_SOURCES}
        COMMENT "Building CHEngine.Managed C# Library"
        VERBATIM
    )

    add_custom_target(CHEngine_Managed ALL DEPENDS "${MANAGED_DLL_OUTPUT}")

    if(TARGET CoralManaged)
        add_dependencies(CHEngine_Managed CoralManaged)
    endif()
    
    add_dependencies(engine_scripting CHEngine_Managed)

    add_custom_command(TARGET CHEngine_Managed POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CORAL_MANAGED_DIR}/Coral.Managed.dll"
            "${CORAL_MANAGED_DIR}/Coral.Managed.runtimeconfig.json"
            "${CORAL_MANAGED_DIR}/Coral.Managed.deps.json"
            "${CORAL_MANAGED_DIR}/Coral.Managed.pdb"
            "${MANAGED_OUTPUT_DIR}/"
        COMMENT "Deploying Coral.Managed binaries"
        VERBATIM
    )
else()
    message(WARNING "dotnet executable not found! Cannot build CHEngine.Managed C# library.")
endif()

# ── Build Game Projects ──────────────────────────────────────────────────────
# Automatically find and include all projects in the game/ directory.
# This avoids manual if/else blocks and allows adding new games easily.
file(GLOB GAME_PROJECT_LIST RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/game" "game/*")
foreach(GAME_PROJECT_DIR ${GAME_PROJECT_LIST})
    set(GAME_PATH "${CMAKE_CURRENT_SOURCE_DIR}/game/${GAME_PROJECT_DIR}")
    if(IS_DIRECTORY "${GAME_PATH}" AND EXISTS "${GAME_PATH}/CMakeLists.txt")
        # If CH_ACTIVE_GAME is defined, only add the specific one to save config time (optional)
        if(NOT CH_ACTIVE_GAME OR CH_ACTIVE_GAME STREQUAL GAME_PROJECT_DIR)
            add_subdirectory("game/${GAME_PROJECT_DIR}")
        endif()
    endif()
endforeach()
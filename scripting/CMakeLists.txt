chained_add_engine_module(engine_scripting
    SOURCES
        scriptengine.h
        scriptengine.cpp
        scriptengine_services.h
        scriptengine_services.cpp
        script_glue.h
        script_glue.cpp
        script_glue_internal.h
        script_glue_system.cpp
        script_glue_input.cpp
        script_glue_network.cpp
        script_glue_scene.cpp
        script_glue_entity.cpp
        script_glue_camera.cpp
        script_glue_ui.cpp
        script_glue_audio.cpp
    DEPENDS
        engine_core
        engine_audio
        engine_physics
        ChainedEngine::External::YamlCpp
        ChainedEngine::External::EnTT
        ChainedEngine::External::GLM
        ChainedEngine::External::Coral
        ChainedEngine::External::Miniaudio
)

if(CH_BUILD_PRESET_NAMES_INCLUDE_DIR)
    target_include_directories(engine_scripting PRIVATE
        ${CH_BUILD_PRESET_NAMES_INCLUDE_DIR}
    )
endif()

if(MSVC)
    target_compile_options(engine_scripting PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_scripting PRIVATE -Wa,-mbig-obj)
endif()

# ── C# Managed API Build ──────────────────────────────────────────────────
# This builds the core engine scripting API (.dll) which the game depends on.
set(MANAGED_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/build_scripts/build_managed.py")
set(MANAGED_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managed/CHEngine.Managed.csproj")
set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/vendor/coral")

file(GLOB_RECURSE MANAGED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
list(APPEND MANAGED_SOURCES "${MANAGED_PROJECT_PATH}")

set(MANAGED_DLL_OUTPUT "${MANAGED_OUTPUT_DIR}/CHEngine.Managed.dll")

add_custom_command(
    OUTPUT "${MANAGED_DLL_OUTPUT}"
    COMMAND "${CH_PYTHON_EXECUTABLE}" "${MANAGED_BUILD_SCRIPT}" build-managed
            --project "${MANAGED_PROJECT_PATH}"
            --output "${MANAGED_OUTPUT_DIR}"
            --coral-dir "${CORAL_MANAGED_DIR}"
            --configuration $<CONFIG>
            --write-props
            --copy-coral
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/managed"
    DEPENDS ${MANAGED_SOURCES} "${MANAGED_BUILD_SCRIPT}"
    COMMENT "Building CHEngine.Managed C# Library"
    VERBATIM
)

add_custom_target(CHEngine_Managed ALL DEPENDS "${MANAGED_DLL_OUTPUT}")

if(TARGET CoralManaged)
    add_dependencies(CHEngine_Managed CoralManaged)
endif()

add_dependencies(engine_scripting CHEngine_Managed)
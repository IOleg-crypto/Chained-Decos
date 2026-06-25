chained_add_engine_module(engine_scripting
    SOURCES
        scriptengine.h
        scriptengine.cpp
        scriptengine_services.h
        scriptengine_services.cpp
        scene_scripting_manager.h
        scene_scripting_manager.cpp
        script_glue.h
        script_glue.cpp
        script_glue_internal.h
        script_glue_system.cpp
        script_glue_input.cpp
        #script_glue_network.cpp
        script_glue_scene.cpp
        script_glue_entity.cpp
        script_glue_camera.cpp
        script_glue_ui.cpp
        script_glue_audio.cpp
    DEPENDS
        engine_core
        engine_audio
        engine_physics
        engine_imgui
        engine_graphics
        Coral.Native
)

target_include_directories(engine_scripting PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty/coral/Coral.Native/Include
    ${CMAKE_SOURCE_DIR}/thirdparty/coral/NetCore
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
# Builds the core engine scripting API (.dll) which the game depends on.
set(MANAGED_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managed/CHEngine.Managed.csproj")
set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

find_program(DOTNET_EXE NAMES dotnet)
if(NOT DOTNET_EXE)
    message(WARNING "dotnet not found - CHEngine.Managed.dll will not be built")
endif()

file(GLOB_RECURSE MANAGED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
list(APPEND MANAGED_SOURCES "${MANAGED_PROJECT_PATH}")

set(MANAGED_DLL_OUTPUT "${MANAGED_OUTPUT_DIR}/CHEngine.Managed.dll")

add_custom_command(
    OUTPUT  "${MANAGED_DLL_OUTPUT}"
    COMMAND "${DOTNET_EXE}" build "${MANAGED_PROJECT_PATH}"
            --output "${MANAGED_OUTPUT_DIR}"
            --configuration $<CONFIG>
            --nologo
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
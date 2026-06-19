chained_add_engine_module(engine_scripting
    EXCLUDE_FILES
        "script_glue_network.cpp"
    DEPENDS
        engine_core
        engine_audio
        engine_physics
        yaml-cpp
        engine_external_entt
        glm
        Coral.Native
        engine_external_miniaudio
        engine_external_imgui
)



if(MSVC)
    target_compile_options(engine_scripting PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_scripting PRIVATE -Wa,-mbig-obj)
endif()

# ── C# Managed API Build ──────────────────────────────────────────────────
# This builds the core engine scripting API (.dll) which the game depends on.
set(MANAGED_BUILD_SCRIPT "${CMAKE_SOURCE_DIR}/tools/sync_resources.py")
set(MANAGED_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managed/Chained.Managed.csproj")
set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/vendor/coral")

file(GLOB_RECURSE MANAGED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
list(APPEND MANAGED_SOURCES "${MANAGED_PROJECT_PATH}")

set(MANAGED_DLL_OUTPUT "${MANAGED_OUTPUT_DIR}/Chained.Managed.dll")

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
    COMMENT "Building Chained.Managed C# Library"
    VERBATIM
)

add_custom_target(CHEngine_Managed ALL DEPENDS "${MANAGED_DLL_OUTPUT}")

if(TARGET CoralManaged)
    add_dependencies(CHEngine_Managed CoralManaged)
endif()

add_dependencies(engine_scripting CHEngine_Managed)
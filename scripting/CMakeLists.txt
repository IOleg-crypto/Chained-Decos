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


if(MSVC)
    target_compile_options(engine_scripting PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_scripting PRIVATE -Wa,-mbig-obj)
endif()

# ── Generate ManagedDependencies.props for C# build ───────────────────────────
# This tells MSBuild where to find Coral.Managed.dll. The path changes per
# build preset (windows-clang, windows-gcc, etc.), so we generate it from a
# template at configure time using the correct CMAKE_BINARY_DIR.
set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/vendor/coral")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/managed/ManagedDependencies.props.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/managed/ManagedDependencies.props"
    @ONLY
)

set(MANAGED_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/managed")
set(MANAGED_PROJECT_PATH "${MANAGED_PROJECT_DIR}/CHEngine.Managed.csproj")

set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/vendor/coral")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(BUILD_HELPER_SCRIPT "${CMAKE_SOURCE_DIR}/tools/sync_resources.py") 

file(GLOB_RECURSE MANAGED_SOURCES "${MANAGED_PROJECT_DIR}/src/*.cs")

set(MANAGED_DLL_OUTPUT "${MANAGED_OUTPUT_DIR}/CHEngine.Managed.dll")

add_custom_command(
    OUTPUT  "${MANAGED_DLL_OUTPUT}"
    COMMAND Python3::Interpreter "${BUILD_HELPER_SCRIPT}" build-managed
            --project "${MANAGED_PROJECT_PATH}"
            --output "${MANAGED_OUTPUT_DIR}"
            --coral-dir "${CORAL_MANAGED_DIR}"
            --configuration $<CONFIG>
            --copy-coral   
            --write-props  
            --parallel     
    WORKING_DIRECTORY "${MANAGED_PROJECT_DIR}"
    DEPENDS ${MANAGED_SOURCES} "${MANAGED_PROJECT_PATH}"
    COMMENT "Building CHEngine.Managed C# Library"
    VERBATIM
)

# 3. Створення таргету та зв'язування залежностей
add_custom_target(CHEngine_Managed ALL DEPENDS "${MANAGED_DLL_OUTPUT}")

if(TARGET Coral.Native)
    add_dependencies(CHEngine_Managed Coral.Native)
endif()

add_dependencies(engine_scripting CHEngine_Managed)
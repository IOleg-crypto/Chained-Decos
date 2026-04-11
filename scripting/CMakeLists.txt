add_library(scripting STATIC
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
    script_glue_gameplay.cpp
    scene_scripting.h
    scene_scripting.cpp
)

target_include_directories(scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/coral/Coral.Native/Include
    ${CMAKE_SOURCE_DIR}/include/glfw/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/miniaudio
)

target_link_libraries(scripting PUBLIC 
    engine
    yaml-cpp
    EnTT
    Coral.Native
)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(scripting)
endif()

if(MSVC)
    target_compile_options(scripting PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(scripting PRIVATE -Wa,-mbig-obj)
endif()

# ── C# Managed API Build ──────────────────────────────────────────────────
# This builds the core engine scripting API (.dll) which the game depends on.

find_program(DOTNET_EXECUTABLE dotnet)
if(DOTNET_EXECUTABLE)
    set(MANAGED_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managed/CHEngine.Managed.csproj")
    set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
    set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/include/coral/cmake")
    
    # Generate ManagedDependencies.props to provide paths to the .csproj
    # Only regenerate if content changes using copy_if_different
    file(RELATIVE_PATH RELATIVE_CORAL_MANAGED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/managed" "${CORAL_MANAGED_DIR}")
    file(TO_NATIVE_PATH "${RELATIVE_CORAL_MANAGED_DIR}" NATIVE_RELATIVE_CORAL_MANAGED_DIR)
    
    set(PROPS_CONTENT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/managed/ManagedDependencies.props")
    set(PROPS_TEMPLATE_CONTENT "<Project>\n  <PropertyGroup>\n    <CoralManagedDir>$(MSBuildThisFileDirectory)${NATIVE_RELATIVE_CORAL_MANAGED_DIR}</CoralManagedDir>\n  </PropertyGroup>\n</Project>")
    
    # Write to temp file and only update if different (prevents unnecessary rebuilds)
    file(WRITE "${CMAKE_BINARY_DIR}/.cmake_temp_props" "${PROPS_TEMPLATE_CONTENT}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/.cmake_temp_props" "${PROPS_CONTENT_FILE}")
    
    # Collect all C# source files and lock file for dependency tracking
    file(GLOB_RECURSE MANAGED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
    list(APPEND MANAGED_SOURCES "${MANAGED_PROJECT_PATH}")
    list(APPEND MANAGED_SOURCES "${PROPS_CONTENT_FILE}")
    
    # 1. Collect all local managed sources for dependency tracking
    file(GLOB_RECURSE MANAGED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
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
    
    add_dependencies(scripting CHEngine_Managed)

    # Deploy Coral.Managed files
    set(CORAL_FILES 
        "Coral.Managed.dll" 
        "Coral.Managed.runtimeconfig.json"
        "Coral.Managed.deps.json"
        "Coral.Managed.pdb"
    )

    add_custom_command(TARGET CHEngine_Managed POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/include/coral/cmake/Coral.Managed.dll"
            "${CMAKE_BINARY_DIR}/include/coral/cmake/Coral.Managed.runtimeconfig.json"
            "${CMAKE_BINARY_DIR}/include/coral/cmake/Coral.Managed.deps.json"
            "${CMAKE_BINARY_DIR}/include/coral/cmake/Coral.Managed.pdb"
            "${CMAKE_BINARY_DIR}/bin/"
        COMMENT "Deploying Coral.Managed binaries"
        VERBATIM
    )
else()
    message(WARNING "dotnet executable not found! Cannot build CHEngine.Managed C# library.")
endif()
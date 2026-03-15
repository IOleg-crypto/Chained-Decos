add_library(scripting STATIC
    scriptengine.h
    scriptengine.cpp
    script_glue.h
    script_glue.cpp
    scene_scripting.h
    scene_scripting.cpp
    script_file_system.h
    script_file_system.cpp
)

target_include_directories(scripting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/coral/Coral.Native/Include
)

target_link_libraries(scripting PUBLIC 
    engine
    raylib 
    yaml-cpp
    EnTT
    Coral.Native
)

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
    file(RELATIVE_PATH RELATIVE_CORAL_MANAGED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/managed" "${CORAL_MANAGED_DIR}")
    file(TO_NATIVE_PATH "${RELATIVE_CORAL_MANAGED_DIR}" NATIVE_RELATIVE_CORAL_MANAGED_DIR)
    
    set(PROPS_CONTENT 
"<Project>
  <PropertyGroup>
    <CoralManagedDir>$(MSBuildThisFileDirectory)${NATIVE_RELATIVE_CORAL_MANAGED_DIR}</CoralManagedDir>
  </PropertyGroup>
</Project>")
    
    set(PROPS_CONTENT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/managed/ManagedDependencies.props")
    file(WRITE "${PROPS_CONTENT_FILE}" "${PROPS_CONTENT}")
    
    # Collect all C# source files for dependency tracking
    file(GLOB_RECURSE MANAGED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/managed/src/*.cs")
    list(APPEND MANAGED_SOURCES "${MANAGED_PROJECT_PATH}")

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

    # Deploy Coral.Managed.dll
    add_custom_command(TARGET scripting POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/include/coral/cmake/Coral.Managed.dll"
        "${CMAKE_BINARY_DIR}/bin/Coral.Managed.dll"
        COMMENT "Deploying Coral.Managed.dll to bin/"
        VERBATIM
    )
else()
    message(WARNING "dotnet executable not found! Cannot build CHEngine.Managed C# library.")
endif()
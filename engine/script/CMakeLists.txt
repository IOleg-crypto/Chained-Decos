add_library(engine_script OBJECT
    scriptengine.h
    scriptengine.cpp
    script_glue.cpp
)

target_include_directories(engine_script PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${yaml-cpp_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/entt/src
)

target_link_libraries(engine_script PUBLIC 
    raylib 
    yaml-cpp
    EnTT
    Coral.Native
)

if(MSVC)
    target_compile_options(engine_script PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_script PRIVATE -Wa,-mbig-obj)
endif()

# ── C# Managed API Build ──────────────────────────────────────────────────
# This builds the core engine scripting API (.dll) which the game depends on.

find_program(DOTNET_EXECUTABLE dotnet)
if(DOTNET_EXECUTABLE)
    set(MANAGED_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managed/CHEngine.Managed.csproj")
    set(MANAGED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
    set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/include/coral/cmake")
    
    # Generate ManagedDependencies.props to provide paths to the .csproj
    # Use relative paths for better portability between machines and configurations
    file(RELATIVE_PATH RELATIVE_CORAL_MANAGED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/managed" "${CORAL_MANAGED_DIR}")
    
    # Use native paths (backslashes on Windows) for the MSBuild property
    file(TO_NATIVE_PATH "${RELATIVE_CORAL_MANAGED_DIR}" NATIVE_RELATIVE_CORAL_MANAGED_DIR)
    
    set(PROPS_CONTENT 
"<Project>
  <PropertyGroup>
    <CoralManagedDir>$(MSBuildThisFileDirectory)${NATIVE_RELATIVE_CORAL_MANAGED_DIR}</CoralManagedDir>
  </PropertyGroup>
</Project>")
    
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/managed/ManagedDependencies.props" "${PROPS_CONTENT}")
    
    add_custom_target(CHEngine_Managed ALL
        COMMAND ${DOTNET_EXECUTABLE} build "${MANAGED_PROJECT_PATH}" -c $<CONFIG> --no-incremental --output "${MANAGED_OUTPUT_DIR}" -p:CoralManagedDir="${CORAL_MANAGED_DIR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/managed"
        COMMENT "Building CHEngine.Managed C# Library"
        VERBATIM
    )

    # Ensure CHEngine_Managed builds AFTER Coral.Managed to avoid dotnet race condition
    if(TARGET CoralManaged)
        add_dependencies(CHEngine_Managed CoralManaged)
    endif()
    
    # Ensure the engine script object files are built after/with the managed lib if needed
    # (Though they are mostly independent, it's good for ordering)
    add_dependencies(engine_script CHEngine_Managed)
else()
    message(WARNING "dotnet executable not found! Cannot build CHEngine.Managed C# library.")
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_script)
endif()
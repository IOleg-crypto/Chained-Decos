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

    set(CORAL_MANAGED_DIR "${CMAKE_BINARY_DIR}/vendor/coral")
    set(SCRIPT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/scripts/${TARGET_NAME}")
    set(SCRIPT_DLL_PATH "${SCRIPT_OUTPUT_DIR}/${TARGET_NAME}.dll")

    add_custom_target(${SCRIPT_TARGET}
        COMMAND "${CH_PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/sync_resources.py" build-managed
            --project "${FULL_CSPROJ_PATH}"
            --configuration $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:>>,Debug,Release>
            --output "${SCRIPT_OUTPUT_DIR}"
            --coral-dir "${CORAL_MANAGED_DIR}"
            --parallel
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building C# Scripts for ${TARGET_NAME} (incremental)"
        BYPRODUCTS "${SCRIPT_DLL_PATH}"
        VERBATIM
    )
    
    # Ensure scripts build AFTER Chained_Managed to avoid dotnet race condition
    if(TARGET Chained_Managed)
        add_dependencies(${SCRIPT_TARGET} Chained_Managed)
    endif()
endfunction()

# Helper function to standardize engine sub-module creation
function(chained_add_engine_module TARGET_NAME)
    set(options NO_PCH)
    set(oneValueArgs)
    set(multiValueArgs SOURCES DEPENDS EXCLUDE_DIRS EXCLUDE_FILES)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(GLOB_RECURSE AUTO_SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h" "*.c" "*.hh" "*.hpp")
    if(MODULE_EXCLUDE_DIRS)
        foreach(EXCLUDE_DIR ${MODULE_EXCLUDE_DIRS})
            list(FILTER AUTO_SOURCES EXCLUDE REGEX "${EXCLUDE_DIR}/.*")
        endforeach()
    endif()

    if(MODULE_EXCLUDE_FILES)
        foreach(EXCLUDE_FILE ${MODULE_EXCLUDE_FILES})
            list(FILTER AUTO_SOURCES EXCLUDE REGEX ".*${EXCLUDE_FILE}$")
        endforeach()
    endif()

    if(MODULE_SOURCES)
        list(APPEND AUTO_SOURCES ${MODULE_SOURCES})
    endif()

    add_library(${TARGET_NAME} ${SUBMODULE_LIB_TYPE} ${AUTO_SOURCES})
    
    target_include_directories(${TARGET_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
    )

    if(MODULE_DEPENDS)
        target_link_libraries(${TARGET_NAME} PUBLIC ${MODULE_DEPENDS})
    endif()

    if(COMMAND apply_engine_optimizations)
        if(MODULE_NO_PCH)
            apply_engine_optimizations(${TARGET_NAME} NO_PCH)
        else()
            apply_engine_optimizations(${TARGET_NAME})
        endif()
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

    # 1. Locate the entry point (main.cpp) (OPTIONAL)
    set(ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    set(HAS_ENTRY_POINT OFF)
    if(EXISTS "${ENTRY_SOURCE}")
        set(HAS_ENTRY_POINT ON)
    endif()

    # 2. Compile sources once using an OBJECT library (saves RAM and compile time)
    if(GAME_SOURCES)
        foreach(LIB_TARGET IN ITEMS ${TARGET_NAME} ${TARGET_NAME}Module)
            if(LIB_TARGET STREQUAL ${TARGET_NAME})
                add_library(${LIB_TARGET} STATIC ${GAME_SOURCES})
                # Rename static lib so its .lib doesn't clash with the IMPLIB generated
                # by the exe on MSVC (both would be ChainedDecos.lib otherwise → LNK1181).
                set_target_properties(${LIB_TARGET} PROPERTIES OUTPUT_NAME "${TARGET_NAME}Game")
            else()
                add_library(${LIB_TARGET} SHARED ${GAME_SOURCES})
                target_compile_definitions(${LIB_TARGET} PRIVATE GAME_BUILD_DLL)
                # Avoid conflict with static library .lib on Windows
                set_target_properties(${LIB_TARGET} PROPERTIES OUTPUT_NAME "${TARGET_NAME}Module")
            endif()
            target_link_libraries(${LIB_TARGET} PUBLIC ChainedEngine::Framework)
            _chained_configure_game_target(${LIB_TARGET})
        endforeach()
    endif()

    # Create a wrapper target for the game to attach commands to
    add_custom_target(${TARGET_NAME}GameTarget)

    # 3. Create the EXECUTABLE target
    if(HAS_ENTRY_POINT)
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

        add_dependencies(${TARGET_NAME}GameTarget ${TARGET_NAME}Exe)
    endif()

    # 4. Handle C# Script Building
    if(GAME_CSHARP_PROJECT)
        chained_add_csharp_scripts(${TARGET_NAME} "${GAME_CSHARP_PROJECT}")
        add_dependencies(${TARGET_NAME}GameTarget "BuildScripts_${TARGET_NAME}")
        if(HAS_ENTRY_POINT)
            add_dependencies(${TARGET_NAME}Exe "BuildScripts_${TARGET_NAME}")
        endif()
    endif()

    # 5. Installation
    if(HAS_ENTRY_POINT)
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
    endif()

    message(STATUS "Configured Project: ${GAME_PROJECT_GAME} (Output=${TARGET_NAME})")
endfunction()

# Copy engine resources (shaders, fonts, icons, config) to a target's output directory
function(ch_add_resource_sync TARGET)
    set(RESOURCES_SRC "${CMAKE_SOURCE_DIR}/resources")
    set(RESOURCES_DST "$<TARGET_FILE_DIR:${TARGET}>/resources")

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RESOURCES_DST}"
        COMMAND ${CMAKE_COMMAND} -DSOURCE="${RESOURCES_SRC}" -DDEST="${RESOURCES_DST}" -P "${CMAKE_SOURCE_DIR}/cmake/CopyIfDifferent.cmake"
        COMMENT "Syncing engine resources to ${RESOURCES_DST}..."
    )
endfunction()



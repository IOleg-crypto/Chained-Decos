# Helper function to create a standalone game project
# Usage: chained_add_game(TARGET_NAME DISPLAY_NAME CSHARP_PROJECT_PATH [native_sources...])
function(chained_add_game TARGET_NAME DISPLAY_NAME CSHARP_PROJECT_PATH)
    set(NATIVE_SOURCES ${ARGN})

    # 1. Locate the entry point (main.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
        set(ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    else()
        message(FATAL_ERROR "chained_add_game: Could not find src/main.cpp for ${TARGET_NAME}")
    endif()

    # 2. Create C++ libraries ONLY if native sources are provided
    if(NATIVE_SOURCES)
        # Create a STATIC LIBRARY for game logic (for static linking/tests)
        add_library(${TARGET_NAME} STATIC ${NATIVE_SOURCES})
        target_link_libraries(${TARGET_NAME} PUBLIC engine)
        
        # Create a SHARED LIBRARY for dynamic loading (Hot Reload / Native Runtime)
        add_library(${TARGET_NAME}Module SHARED ${NATIVE_SOURCES})
        target_link_libraries(${TARGET_NAME}Module PUBLIC engine)
        target_compile_definitions(${TARGET_NAME}Module PRIVATE GAME_BUILD_DLL)
        set_target_properties(${TARGET_NAME}Module PROPERTIES OUTPUT_NAME "${TARGET_NAME}")
        
        if(COMMAND apply_engine_optimizations)
            apply_engine_optimizations(${TARGET_NAME})
            apply_engine_optimizations(${TARGET_NAME}Module)
        endif()
    endif()

    # 3. Create the EXECUTABLE target
    add_executable(${TARGET_NAME}Exe ${ENTRY_SOURCE})
    
    # Link against dependencies
    target_link_libraries(${TARGET_NAME}Exe PRIVATE RuntimeCore)
    if(NATIVE_SOURCES)
         target_link_libraries(${TARGET_NAME}Exe PRIVATE ${TARGET_NAME})
    endif()
    
    # Set the output name and metadata
    set_target_properties(${TARGET_NAME}Exe PROPERTIES OUTPUT_NAME "${TARGET_NAME}")
    set_target_properties(${TARGET_NAME}Exe PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    
    # Pass metadata to the app
    target_compile_definitions(${TARGET_NAME}Exe PRIVATE 
        GAME_BUILD_EXE
        CH_PROJECT_NAME="${DISPLAY_NAME}"
    )

    if(COMMAND apply_engine_optimizations)
        apply_engine_optimizations(${TARGET_NAME}Exe)
    endif()

    # 4. Configure include directories
    target_include_directories(${TARGET_NAME}Exe PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}
    )
    if(TARGET ${TARGET_NAME})
        target_include_directories(${TARGET_NAME} PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}
        )
    endif()
    if(TARGET ${TARGET_NAME}Module)
        target_include_directories(${TARGET_NAME}Module PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}
        )
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

    # 6. Handle C# Script Building if provided
    if(CSHARP_PROJECT_PATH)
        set(SCRIPT_TARGET "BuildScripts_${TARGET_NAME}")
        if(NOT TARGET ${SCRIPT_TARGET})
            set(FULL_CSPROJ_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${CSHARP_PROJECT_PATH}")
            add_custom_target(${SCRIPT_TARGET} ALL
                COMMAND dotnet build "${FULL_CSPROJ_PATH}" -c $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:>>,Debug,Release>
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                COMMENT "Building C# Scripts for ${TARGET_NAME}"
            )
            add_dependencies(${TARGET_NAME}Exe ${SCRIPT_TARGET})
        endif()
    endif()

    message(STATUS "Configured Project: ${DISPLAY_NAME} (Exe=${TARGET_NAME}Exe, Output=${TARGET_NAME})")
endfunction()

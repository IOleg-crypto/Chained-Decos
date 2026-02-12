# Helper function to create a standalone game executable
function(chained_add_game TARGET_NAME)
    # 1. Locate the entry point (main.cpp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
        set(ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    else()
        message(FATAL_ERROR "chained_add_game: Could not find src/main.cpp for ${TARGET_NAME}")
    endif()

    # 2. Create a STATIC LIBRARY for game logic
    # This target will be linked by the Editor and the Game EXE
    # We use the original name for the library so existing link calls work
    add_library(${TARGET_NAME} STATIC ${ARGN})
    target_link_libraries(${TARGET_NAME} PUBLIC engine)
    
    if(COMMAND apply_engine_optimizations)
        apply_engine_optimizations(${TARGET_NAME})
    endif()

    # 3. Create the EXECUTABLE target
    # We name the target ${TARGET_NAME}Exe to avoid collision with the lib target
    add_executable(${TARGET_NAME}Exe ${ENTRY_SOURCE})
    
    # Link against the game logic library and runtime core
    target_link_libraries(${TARGET_NAME}Exe PRIVATE ${TARGET_NAME} RuntimeCore)
    
    # Set the output name to the original TARGET_NAME (e.g., ChainedDecosGame.exe)
    set_target_properties(${TARGET_NAME}Exe PROPERTIES OUTPUT_NAME "${TARGET_NAME}")
    
    # Set working directory for debugging to the project root
    set_target_properties(${TARGET_NAME}Exe PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    
    # Define macros
    target_compile_definitions(${TARGET_NAME}Exe PRIVATE GAME_BUILD_EXE)

    if(COMMAND apply_engine_optimizations)
        apply_engine_optimizations(${TARGET_NAME}Exe)
    endif()

    message(STATUS "Configured Game: Lib=${TARGET_NAME}, Exe=${TARGET_NAME}Exe (Output=${TARGET_NAME})")
endfunction()

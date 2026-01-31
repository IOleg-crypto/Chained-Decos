# Helper function to create a standalone game executable
function(chained_add_game TARGET_NAME PROJECT_DIR ENTRY_SOURCE)
    # 1. Create the executable using the project-specific entry point
    add_executable(${TARGET_NAME} ${ENTRY_SOURCE})

    # 2. Link against the Runtime wrapper and the game's own logic library
    # Note: ChainedDecosGame is currently hardcoded for the existing project,
    # but in a multi-project setup, you'd pass the logic library as an argument too.
    # For now, we'll assume the project library is named after its directory or passed.
    
    # We'll use a naming convention: ${TARGET_NAME}Logic or similar?
    # Actually, let's just make it flexible by allowing multiple libraries.
    # For now, let's keep it simple for the user.
    
    target_link_libraries(${TARGET_NAME} PRIVATE RuntimeCore ${ARGN})
    
    # 3. Apply engine optimizations
    if(COMMAND apply_engine_optimizations)
        apply_engine_optimizations(${TARGET_NAME})
    endif()

    if(COMMAND apply_engine_optimizations)
        apply_engine_optimizations(${TARGET_NAME})
    endif()

    message(STATUS "Configured Game Runtime: ${TARGET_NAME}")
endfunction()

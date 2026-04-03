# Define the Game module as a Standalone Executable
# Signature: TARGET_NAME DISPLAY_NAME CSHARP_PROJECT_PATH [native_sources...]
chained_add_game(ChainedDecosGame 
    "Chained Decos" 
    "scripts/ChainedDecos.Scripts.csproj"
)

# Copy engine resources
add_custom_command(TARGET ChainedDecosGameExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:ChainedDecosGameExe>/resources"
    COMMENT "Copying engine resources..."
)

# Copy game assets
add_custom_command(TARGET ChainedDecosGameExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:ChainedDecosGameExe>/assets"
    COMMENT "Copying game assets..."
)

# Additional configurations can go here if needed

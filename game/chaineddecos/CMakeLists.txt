# Define the Game module as a Standalone Executable
# Signature: TARGET_NAME PROJECT_GAME [CSHARP_PROJECT_PATH] [native_sources...]
chained_add_game(ChainedDecosGame 
    PROJECT_GAME ChainedDecos
    CSHARP_PROJECT "scripts/ChainedDecos.Scripts.csproj"
)

# Copy engine resources (only if source files changed)
add_custom_command(TARGET ChainedDecosGameExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:ChainedDecosGameExe>/resources"
    COMMENT "Copying engine resources..."
)

# Copy game assets (only if source files changed)
add_custom_command(TARGET ChainedDecosGameExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:ChainedDecosGameExe>/assets"
    COMMENT "Copying game assets..."
)
# Additional configurations can go here if needed

# Define the Game module as a Standalone Executable
# Signature: TARGET_NAME PROJECT_GAME [CSHARP_PROJECT_PATH] [native_sources...]
chained_add_game(ChainedDecos
    PROJECT_GAME ChainedDecos
    CSHARP_PROJECT "scripts/ChainedDecos.Scripts.csproj"
)

# Use centralized engine resources
chained_add_engine_resources_copy()
add_dependencies(ChainedDecosExe EngineResources)

# Copy game assets (only if source files changed)
set(GAME_ASSET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(GAME_ASSET_DST_DIR "$<TARGET_FILE_DIR:ChainedDecosExe>/assets")

add_custom_command(TARGET ChainedDecosExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GAME_ASSET_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -DSOURCE="${GAME_ASSET_SRC_DIR}" -DDEST="${GAME_ASSET_DST_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/CopyIfDifferent.cmake"
    COMMENT "Syncing game assets from ${GAME_ASSET_SRC_DIR} to ${GAME_ASSET_DST_DIR}..."
)
# Additional configurations can go here if needed

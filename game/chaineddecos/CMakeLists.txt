# Define the Game module as a Standalone Executable
# Signature: TARGET_NAME PROJECT_GAME [CSHARP_PROJECT_PATH] [native_sources...]
set(GAME_SOURCES
    src/game_module.cpp
    src/game_script_bindings.cpp
)

chained_add_game(ChainedDecos
    PROJECT_GAME ChainedDecos
    CSHARP_PROJECT "scripts/ChainedDecos.Scripts.csproj"
    SOURCES ${GAME_SOURCES}
)

# Copy game assets (only if source files changed)
set(GAME_ASSET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(GAME_ASSET_DST_DIR "$<TARGET_FILE_DIR:ChainedRuntime>/assets")

add_custom_command(TARGET ChainedDecosGameTarget POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GAME_ASSET_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -DSOURCE="${GAME_ASSET_SRC_DIR}" -DDEST="${GAME_ASSET_DST_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/CopyIfDifferent.cmake"
    COMMENT "Syncing game assets from ${GAME_ASSET_SRC_DIR} to ${GAME_ASSET_DST_DIR}..."
)
# Additional configurations can go here if needed

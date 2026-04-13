# Define the Game module as a Standalone Executable
# Signature: TARGET_NAME PROJECT_GAME [CSHARP_PROJECT_PATH] [native_sources...]
chained_add_game(ChainedDecos
    PROJECT_GAME ChainedDecos
    CSHARP_PROJECT "scripts/ChainedDecos.Scripts.csproj"
)

# Copy engine resources (only if source files changed)
set(CH_RESOURCE_SRC_DIR "${CMAKE_SOURCE_DIR}/resources")
set(CH_RESOURCE_DST_DIR "$<TARGET_FILE_DIR:ChainedDecosExe>/resources")

add_custom_command(TARGET ChainedDecosExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CH_RESOURCE_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CH_RESOURCE_SRC_DIR}" "${CH_RESOURCE_DST_DIR}"
    COMMENT "Copying engine resources from ${CH_RESOURCE_SRC_DIR} to ${CH_RESOURCE_DST_DIR}..."
)

# Copy game assets (only if source files changed)
set(GAME_ASSET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(GAME_ASSET_DST_DIR "$<TARGET_FILE_DIR:ChainedDecosExe>/assets")

add_custom_command(TARGET ChainedDecosExe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GAME_ASSET_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${GAME_ASSET_SRC_DIR}" "${GAME_ASSET_DST_DIR}"
    COMMENT "Copying game assets from ${GAME_ASSET_SRC_DIR} to ${GAME_ASSET_DST_DIR}..."
)
# Additional configurations can go here if needed

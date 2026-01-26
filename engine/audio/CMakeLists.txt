# Register Audio sources to the unified engine target
target_sources(engine PRIVATE
    # Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/sound_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sound_asset.h
)

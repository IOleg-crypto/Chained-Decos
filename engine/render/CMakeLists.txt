# Register Render sources to the unified engine target
target_sources(engine PRIVATE
    # Renderer Core
    ${CMAKE_CURRENT_SOURCE_DIR}/visuals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/visuals.h
    ${CMAKE_CURRENT_SOURCE_DIR}/api_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api_context.h
    ${CMAKE_CURRENT_SOURCE_DIR}/draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/draw_command.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scene_pipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render.h
    ${CMAKE_CURRENT_SOURCE_DIR}/render_types.h

    # Asset Management
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_archive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_archive.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resource_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource_provider.h

    # Assets
    ${CMAKE_CURRENT_SOURCE_DIR}/model_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/model_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/texture_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/texture_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/environment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/environment.h
    ${CMAKE_CURRENT_SOURCE_DIR}/font_asset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/font_asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/asset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/material.h
)

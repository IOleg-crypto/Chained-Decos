add_library(engine_assets ${SUBMODULE_LIB_TYPE}
    asset_manager.cpp
    asset_manager.h
    asset_path_resolver.cpp
    asset_path_resolver.h
    asset_registry.cpp
    asset_registry.h
    asset.h
    asset_loader.h
)

target_include_directories(engine_assets PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_assets 
PUBLIC 
    engine_core
    ChainedEngine::External::YamlCpp
)

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_assets)
endif()

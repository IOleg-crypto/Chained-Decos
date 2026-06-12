chained_add_engine_module(engine_foundation
    NO_PCH
    SOURCES
        base.h
        color.h
        engine_assert.h
        platform_detection.h
        thread_pool.h
        thread_pool.cpp
        timestep.h
        uuid.h
        uuid.cpp
        zstd_compression.h
    DEPENDS
        ChainedEngine::External::spdlog
        ChainedEngine::External::GLM
        ChainedEngine::External::BSThreadPool
        zstd::libzstd_static
)

if(CH_BUILD_PRESET_NAMES_INCLUDE_DIR)
    target_include_directories(engine_foundation PRIVATE
        ${CH_BUILD_PRESET_NAMES_INCLUDE_DIR}
    )
endif()

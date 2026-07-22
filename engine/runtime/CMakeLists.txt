chained_add_engine_module(engine_runtime_core
     SOURCES
        runtime_layer.cpp
        runtime_layer.h
     DEPENDS
        ChainedEngine::Framework
)


target_include_directories(engine_runtime_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


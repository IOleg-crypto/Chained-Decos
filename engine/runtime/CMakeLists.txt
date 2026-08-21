add_library(engine_runtime_core STATIC
    runtime_layer.h
    runtime_layer.cpp
)

target_include_directories(engine_runtime_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_runtime_core PUBLIC
    ChainedEngine::Framework
)

if(TARGET engine_pch)
    target_link_libraries(engine_runtime_core PRIVATE engine_pch)
endif()

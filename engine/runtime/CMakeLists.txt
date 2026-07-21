add_library(RuntimeCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.cpp
)

target_link_libraries(RuntimeCore PUBLIC ChainedEngine::Framework)
target_include_directories(RuntimeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(RuntimeCore)
endif()

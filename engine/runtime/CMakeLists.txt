add_library(RuntimeCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_layer.cpp
)

target_link_libraries(RuntimeCore PUBLIC ChainedEngine::Framework)
target_include_directories(RuntimeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(RuntimeCore PRIVATE
    <string>
    <vector>
    <memory>
    <filesystem>
    "${CMAKE_SOURCE_DIR}/engine/app/application.h"
)

add_library(model STATIC
    Model.cpp
    Model.h
    ModelInstance.cpp
    ModelInstance.h
    Animation.cpp
    Animation.h
    ModelConfig.h
    JsonHelper.cpp
    JsonHelper.h
    ModelCache.cpp
    ModelCache.h
)

target_include_directories(model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Engine
    $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(model PUBLIC
    raylib
    nlohmann_json::nlohmann_json
    collision
)

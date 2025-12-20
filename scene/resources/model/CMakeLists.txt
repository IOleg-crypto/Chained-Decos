set(MODEL_SOURCES
        core/Model.cpp
        core/Model.h
        core/ModelInstance.cpp
        core/ModelInstance.h
        interfaces/IModelLoader.h
        animation/animation.cpp
        animation/animation.h
        config/ModelConfig.h
        config/ModelConfig.cpp
        parser/jsonParser.cpp
        parser/jsonParser.h
        parser/jsonHelper.cpp
        cache/ModelCache.cpp
        cache/ModelCache.h
        utils/ModelAnalyzer.cpp
        utils/ModelAnalyzer.h
)

add_library(model STATIC ${MODEL_SOURCES})

target_include_directories(model PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/Engine
        $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(model PUBLIC
         raylib
         nlohmann_json::nlohmann_json
         collision
         map
         color
)

set_target_properties(model PROPERTIES POSITION_INDEPENDENT_CODE ON)



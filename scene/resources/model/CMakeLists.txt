set(MODEL_SOURCES
        Model.cpp
        Model.h
        ModelInstance.cpp
        ModelInstance.h
        interfaces/IModelLoader.h
        Animation.cpp
        Animation.h
        ModelConfig.h
        ModelConfig.cpp
        JsonParser.cpp
        JsonParser.h
        JsonHelper.cpp
        ModelCache.cpp
        ModelCache.h
        ModelAnalyzer.cpp
        ModelAnalyzer.h
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



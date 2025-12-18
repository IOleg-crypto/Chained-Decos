set(MODEL_SOURCES
        Core/Model.cpp
        Core/Model.h
        Core/ModelInstance.cpp
        Core/ModelInstance.h
        Interfaces/IModelLoader.h
        Animation/Animation.cpp
        Animation/Animation.h
        Config/ModelConfig.h
        Config/ModelConfig.cpp
        Parser/JsonParser.cpp
        Parser/JsonParser.h
        Parser/JsonHelper.cpp
        Cache/ModelCache.cpp
        Cache/ModelCache.h
        Utils/ModelAnalyzer.cpp
        Utils/ModelAnalyzer.h
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

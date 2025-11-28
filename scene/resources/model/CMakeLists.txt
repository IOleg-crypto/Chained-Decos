add_library(model STATIC
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

# Add PROJECT_ROOT_DIR definition for consistent path handling
target_compile_definitions(model PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

target_include_directories(model PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/Engine
        $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(model PUBLIC
         raylib
         nlohmann_json
         collision
         map
         color
)

set_target_properties(model PROPERTIES POSITION_INDEPENDENT_CODE ON)

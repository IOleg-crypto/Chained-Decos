set(MODEL_SOURCES
        model.cpp
        model.h
        model_instance.cpp
        model_instance.h
        animation.cpp
        animation.h
        model_config.h
        model_config.cpp
        json_parser.cpp
        json_parser.h
        json_helper.cpp
        model_cache.cpp
        model_cache.h
        model_analyzer.cpp
        model_analyzer.h
)

add_library(model STATIC ${MODEL_SOURCES})

target_include_directories(model PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/Engine
        $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(model PUBLIC
          raylib
          nlohmann_json::nlohmann_json
          collision_core
          map
          color
)

set_target_properties(model PROPERTIES POSITION_INDEPENDENT_CODE ON)

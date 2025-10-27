add_library(model STATIC
        Model.cpp
        Model.h
        ModelInstance.cpp
        ModelInstance.h
        Animation.cpp
        Animation.h
        ModelConfig.h
        ModelConfig.cpp
        JsonHelper.cpp
        JsonHelper.h
        ModelCache.cpp
        ModelCache.h
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
)

set_target_properties(model PROPERTIES POSITION_INDEPENDENT_CODE ON)

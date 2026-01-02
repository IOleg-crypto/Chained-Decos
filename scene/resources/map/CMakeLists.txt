set(MAP_SOURCES
    scene_metadata.cpp
    scene_metadata.h
)

add_library(map STATIC ${MAP_SOURCES})

target_include_directories(map
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/json/include
)

target_link_libraries(map PUBLIC raylib nlohmann_json config)

set_target_properties(map PROPERTIES POSITION_INDEPENDENT_CODE ON)

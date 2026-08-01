add_library(engine_reflection INTERFACE)

target_include_directories(engine_reflection INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_reflection INTERFACE
    engine_common
    reflectcpp
    glm
    engine_external_entt
)

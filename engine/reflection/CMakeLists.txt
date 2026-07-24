add_library(engine_reflection STATIC
    reflection.h
    reflection_rfl.h
    reflection.cpp
)

target_include_directories(engine_reflection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_reflection PUBLIC
    engine_common
    reflectcpp
    glm
    engine_external_entt
)

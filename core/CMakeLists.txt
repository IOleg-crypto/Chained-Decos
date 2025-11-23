)

target_include_directories(core
    PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(core
    PUBLIC
        nlohmann_json
        raylib
)

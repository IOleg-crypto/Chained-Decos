chained_add_engine_module(engine_core
    DEPENDS
        engine_reflection
        yaml-cpp
        engine_external_entt
        glm
        engine_external_stb
        Coral.Native

        reflectcpp
        cereal
        spdlog
        libzstd_static
)

target_include_directories(engine_core PUBLIC 
    "${CMAKE_SOURCE_DIR}/include/reflect-cpp/include"
    "${CMAKE_SOURCE_DIR}/include/cereal/include"
    "${ZSTD_INCLUDE_DIR}"
)

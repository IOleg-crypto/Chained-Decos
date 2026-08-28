add_library(engine_pack STATIC
    dictionary_packer.h
    dictionary_packer.cpp
    dictionary_pack_reader.h
    dictionary_pack_reader.cpp
)

target_include_directories(engine_pack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_pack PUBLIC
    engine_core
    pack-static
    libzstd_static
)

target_include_directories(engine_pack PRIVATE
    "${ZSTD_INCLUDE_DIR}"
)

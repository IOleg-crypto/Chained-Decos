add_library(engine_network STATIC
    network_service.h
    network_service.cpp
    net_packet.h
)

target_include_directories(engine_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_network PUBLIC
    engine_core
    engine_project
    engine_external_entt
    engine_external_network
    glm
    assimp
    Jolt
)

if(MSVC)
    target_compile_options(engine_network PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_network PRIVATE -Wa,-mbig-obj)
endif()

if(TARGET engine_pch)
    target_link_libraries(engine_network PRIVATE engine_pch)
endif()

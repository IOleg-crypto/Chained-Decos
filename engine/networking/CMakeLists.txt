add_library(engine_network STATIC
    network_service.h
    network_service.cpp
    net_packet.h
    upnp_port_mapper.h
    upnp_port_mapper.cpp
)

target_include_directories(engine_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(engine_network PUBLIC
    engine_core
    engine_project
    engine_external_entt
    enet::enet_static
    sodium
    glm
    engine_external_miniupnpc
)

if(WIN32)
    target_link_libraries(engine_network PUBLIC ws2_32)
endif()

if(TARGET engine_pch)
    target_link_libraries(engine_network PRIVATE engine_pch)
endif()

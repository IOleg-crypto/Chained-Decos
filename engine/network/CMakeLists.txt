chained_add_engine_module(engine_network
    SOURCES
        network_service.h
        network_service.cpp
    DEPENDS
        engine_core
        ChainedEngine::External::GNS
)

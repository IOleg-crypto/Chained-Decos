# ENet — Reliable UDP networking library
# Provides: client/server, reliable/unreliable channels, fragmentation.
# Single-header variant (enet.h with ENET_IMPLEMENTATION).
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/enet")
    set(ENET_STATIC ON CACHE BOOL "" FORCE)
    set(ENET_SHARED OFF CACHE BOOL "" FORCE)
    set(ENET_TEST OFF CACHE BOOL "" FORCE)

    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/enet" "${CMAKE_BINARY_DIR}/enet")

    message(STATUS "enet: configured (static lib)")
else()
    message(FATAL_ERROR "enet missing at ${CMAKE_SOURCE_DIR}/thirdparty/enet")
endif()

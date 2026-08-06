# GameNetworkingSockets (Valve) — open-source transport layer for games
# Provides: connection-oriented API, reliable/unreliable channels, encryption.
#
# NOTE: ENABLE_ICE builds the ICE client, but NAT traversal only applies to P2P
# connections and needs a signaling service plus STUN/TURN servers. We connect by
# direct IP, so hosting over the internet requires a forwarded UDP port.
#
# Dependencies: protobuf (built from thirdparty/), abseil-cpp (fetched via FetchContent by protobuf)
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/GameNetworkingSockets")
    set(GNS_DIR "${CMAKE_SOURCE_DIR}/thirdparty/GameNetworkingSockets")

    # ---- 1. Build protobuf from thirdparty/protobuf/ ----
    if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/protobuf")
        set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
        # GNS's install(EXPORT) is unconditional and references libprotobuf, so protobuf
        # must publish its own export set or the generate step fails. Its transitive deps
        # have to join that export set too, otherwise CMake rejects the dangling refs.
        set(protobuf_INSTALL ON CACHE BOOL "" FORCE)
        set(utf8_range_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
        set(utf8_range_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
        set(ABSL_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
        set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
        # GNS compiles .proto files, so protoc (and its libprotoc backing lib) must be built.
        set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "" FORCE)
        set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
        set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(protobuf_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

        # Let abseil be fetched automatically by protobuf via FetchContent
        add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/protobuf ${CMAKE_BINARY_DIR}/protobuf)

        # GNS expects these namespaced targets — create aliases if needed
        if(NOT TARGET protobuf::libprotobuf)
            add_library(protobuf::libprotobuf ALIAS libprotobuf)
        endif()
        if(NOT TARGET protobuf::libprotobuf-lite)
            add_library(protobuf::libprotobuf-lite ALIAS libprotobuf-lite)
        endif()

        message(STATUS "Protobuf: built from thirdparty/protobuf")

        # find_package(Protobuf CONFIG) matches the case of the package name, so both
        # spellings must be set for GNS's `find_package(Protobuf ...)` to resolve here.
        set(Protobuf_DIR "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "" FORCE)
        set(protobuf_DIR "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "" FORCE)
    else()
        # Fallback: find protobuf on system
        find_package(Protobuf QUIET CONFIG)
        if(NOT Protobuf_FOUND)
            find_package(Protobuf REQUIRED)
        endif()
    endif()

    # ---- 2. Configure GNS options ----
    set(BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ENABLE_ICE ON CACHE BOOL "" FORCE)

    if(WIN32)
        set(USE_CRYPTO "BCrypt" CACHE STRING "" FORCE)
    else()
        set(USE_CRYPTO "OpenSSL" CACHE STRING "" FORCE)
    endif()

    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

    # ---- 3. Build GNS ----
    # Our global WIN32_LEAN_AND_MEAN (CompilerSettings.cmake) excludes mmsystem.h from
    # windows.h, but GNS calls timeBeginPeriod/timeEndPeriod. Subdirectories inherit the
    # define at add_subdirectory() time, so drop it just across that call and restore it.
    get_directory_property(_ch_saved_defs COMPILE_DEFINITIONS)
    if(WIN32)
        set(_ch_gns_defs "${_ch_saved_defs}")
        list(REMOVE_ITEM _ch_gns_defs WIN32_LEAN_AND_MEAN)
        set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "${_ch_gns_defs}")
    endif()

    add_subdirectory(${GNS_DIR} ${CMAKE_BINARY_DIR}/gamenetworkingsockets)

    if(WIN32)
        set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "${_ch_saved_defs}")
    endif()

    message(STATUS "GameNetworkingSockets: configured (static lib, crypto=${USE_CRYPTO})")
else()
    message(FATAL_ERROR "GameNetworkingSockets submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/GameNetworkingSockets")
endif()

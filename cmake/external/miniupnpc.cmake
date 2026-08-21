# miniupnpc — lightweight UPnP IGD client
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/miniupnp/miniupnpc")
    set(MINIUPNPC_DIR "${CMAKE_SOURCE_DIR}/thirdparty/miniupnp/miniupnpc")

    # Generate miniupnpcstrings.h (config header)
    file(WRITE "${CMAKE_BINARY_DIR}/generated/miniupnpcstrings.h"
        "#ifndef MINIUPNPCSTRINGS_H_INCLUDED\n"
        "#define MINIUPNPCSTRINGS_H_INCLUDED\n"
        "\n"
        "#define OS_STRING \"${CMAKE_SYSTEM_NAME}\"\n"
        "#define MINIUPNPC_VERSION_STRING \"2.3.0\"\n"
        "\n"
        "#define UPNP_VERSION_MAJOR 1\n"
        "#define UPNP_VERSION_MINOR 1\n"
        "#define UPNP_VERSION_STRING \"UPnP/1.1\"\n"
        "\n"
        "#endif\n"
    )

    add_library(engine_external_miniupnpc STATIC
        ${MINIUPNPC_DIR}/src/miniupnpc.c
        ${MINIUPNPC_DIR}/src/miniwget.c
        ${MINIUPNPC_DIR}/src/minisoap.c
        ${MINIUPNPC_DIR}/src/minixml.c
        ${MINIUPNPC_DIR}/src/igd_desc_parse.c
        ${MINIUPNPC_DIR}/src/upnpcommands.c
        ${MINIUPNPC_DIR}/src/upnperrors.c
        ${MINIUPNPC_DIR}/src/upnpreplyparse.c
        ${MINIUPNPC_DIR}/src/portlistingparse.c
        ${MINIUPNPC_DIR}/src/receivedata.c
        ${MINIUPNPC_DIR}/src/connecthostport.c
        ${MINIUPNPC_DIR}/src/addr_is_reserved.c
        ${MINIUPNPC_DIR}/src/minissdpc.c
        ${MINIUPNPC_DIR}/src/upnpdev.c
    )

    target_include_directories(engine_external_miniupnpc PUBLIC
        ${MINIUPNPC_DIR}/include
    )

    target_include_directories(engine_external_miniupnpc PRIVATE
        ${CMAKE_BINARY_DIR}/generated
    )

    # MINIUPNP_STATICLIB (2 N's) — this is what the header checks
    target_compile_definitions(engine_external_miniupnpc PUBLIC
        MINIUPNP_STATICLIB
    )

    if(WIN32)
        target_link_libraries(engine_external_miniupnpc PUBLIC ws2_32 iphlpapi)
    endif()
else()
    message(FATAL_ERROR "miniupnpc submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/miniupnp")
endif()

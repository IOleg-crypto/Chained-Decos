# ============================================================================
# Chained Engine - GameNetworkingSockets Dependency
# ============================================================================

# Define the location of the library source
set(GNS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/include/GameNetworkingSockets")

if(EXISTS "${GNS_SOURCE_DIR}/CMakeLists.txt")
    # Configuration options for GNS
    set(BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(ENABLE_ICE OFF CACHE BOOL "" FORCE) # Disable ICE to avoid WebRTC dependencies
    
    # Disable tracing to avoid winmeta.h dependency on MinGW
    add_definitions(-DVALVE_DISABLE_TRACELOGGING)
    
    # Protobuf is already fetched and configured in protobuf.cmake
    # find_package(Protobuf) in GNS src/CMakeLists.txt will find our fetched version
    
    # On Windows, we can use BCrypt if OpenSSL is not found
    if(WIN32)
        set(USE_CRYPTO "BCrypt" CACHE STRING "" FORCE)
    endif()

    add_subdirectory("${GNS_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vendor/gamenetworkingsockets" EXCLUDE_FROM_ALL)

    # Create an alias for easier linking
    if(TARGET GameNetworkingSockets_s)
        set(GNS_TARGET GameNetworkingSockets_s)
    elseif(TARGET GameNetworkingSockets_static)
        set(GNS_TARGET GameNetworkingSockets_static)
    elseif(TARGET GameNetworkingSockets)
        set(GNS_TARGET GameNetworkingSockets)
    endif()

    if(GNS_TARGET)
        add_library(ChainedEngine::External::GNS ALIAS ${GNS_TARGET})
        target_include_directories(${GNS_TARGET} INTERFACE "$<BUILD_INTERFACE:${GNS_SOURCE_DIR}/include>")
        
        # MinGW fixes for winmm and timeBeginPeriod
        if(MINGW)
            target_link_libraries(${GNS_TARGET} PRIVATE winmm)
            target_compile_options(${GNS_TARGET} PRIVATE -include "${CMAKE_SOURCE_DIR}/include/mingw_compat.h")
        endif()
    endif()
    
    message(STATUS "GameNetworkingSockets: Integrated from ${GNS_SOURCE_DIR}")
else()
    message(WARNING "GameNetworkingSockets: Source not found at ${GNS_SOURCE_DIR}")
endif()

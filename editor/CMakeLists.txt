
file(GLOB EDITOR_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/panels/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/panels/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/project/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/project/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/undo/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/undo/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/viewport/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/viewport/*.h"
)
# NetworkPanel is only compiled when the networking module is enabled
list(REMOVE_ITEM EDITOR_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/panels/network_panel.cpp")
# The editor executable entry point is built separately
list(REMOVE_ITEM EDITOR_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
if(CH_NETWORKING)
    list(APPEND EDITOR_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/panels/network_panel.cpp")
endif()

add_library(editor_core STATIC ${EDITOR_CORE_SOURCES})

target_link_libraries(editor_core PUBLIC
    ChainedEngine::Framework
)

# Pack library — asset packing for shipped builds
if(TARGET pack-static)
    target_link_libraries(editor_core PUBLIC pack-static)
endif()

# Dictionary pack support — ZSTD dictionary compression for asset packs
if(TARGET engine_pack)
    target_link_libraries(editor_core PUBLIC engine_pack)
endif()

if(TARGET engine_external_basisu_encoder)
    target_link_libraries(editor_core PUBLIC engine_external_basisu_encoder)
endif()

# WinHTTP — used by NetworkPanel to fetch the public IP from api.ipify.org.
# Only needed when the networking module is enabled.
if(WIN32 AND CH_NETWORKING)
    target_link_libraries(editor_core PUBLIC winhttp)
endif()

target_include_directories(editor_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define the Editor executable
add_executable(ChainedEditor ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
        
target_link_libraries(ChainedEditor PRIVATE editor_core)

# Packer utility — build-time tool, not linked to the editor
if(TARGET packer)
    add_dependencies(ChainedEditor packer)
endif()

if(TARGET engine_pch)
    target_link_libraries(editor_core PRIVATE engine_pch)
    target_link_libraries(ChainedEditor PRIVATE engine_pch)
endif()

# Automatically sync resources/ into the correct bin/<Config>/ directory after each build
if(COMMAND ch_add_resource_sync)
    ch_add_resource_sync(ChainedEditor)
endif()

# Installation
install(TARGETS ChainedEditor
    RUNTIME DESTINATION bin COMPONENT Editor
)

# Copy global assets to the install directory if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION assets COMPONENT Runtime)
endif()

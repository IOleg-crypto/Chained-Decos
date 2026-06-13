# Assimp dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/include/assimp/CMakeLists.txt")
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE) # Build as shared to reduce dev link times

    # Set model importer formats
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE INTERNAL "")

    set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE INTERNAL "")
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE INTERNAL "")
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE INTERNAL "")
    
    add_subdirectory("${CMAKE_SOURCE_DIR}/include/assimp" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "assimp submodule missing at ${CMAKE_SOURCE_DIR}/include/assimp")
endif()

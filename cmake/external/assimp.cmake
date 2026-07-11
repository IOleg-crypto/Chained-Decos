# Assimp dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/assimp/CMakeLists.txt")
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
    
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/assimp" EXCLUDE_FROM_ALL)

    # CRITICAL: Assimp's bundled glib (contrib/glib/*.c) declares C-style symbols
    # (gp_state, gp_error, etc.) that conflict with system GLib headers when Unity
    # Build merges all translation units into one. Disable Unity Build for assimp
    # to avoid ODR violations and 'conflicting types' errors in CI.
    if(TARGET assimp)
        set_target_properties(assimp PROPERTIES UNITY_BUILD OFF)
    endif()
else()
    message(FATAL_ERROR "assimp submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/assimp")
endif()

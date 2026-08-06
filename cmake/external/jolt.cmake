# Chained Engine - Jolt Physics Dependency

set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
set(USE_AVX2 ON CACHE BOOL "" FORCE)
set(USE_WERROR OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE) # Required for Linux CI - engine uses RTTI
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/JoltPhysics/Build/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/JoltPhysics/Build" "${CMAKE_BINARY_DIR}/vendor/jolt" EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "JoltPhysics submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/JoltPhysics")
endif()



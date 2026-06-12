# Cereal dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/include/cereal/CMakeLists.txt")
    # Cereal is header-only, so we just need its include target
    set(JUST_INSTALL_CEREAL ON CACHE BOOL "" FORCE)
    set(BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
    set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SKIP_PORTABILITY_TEST ON CACHE BOOL "" FORCE)

    add_subdirectory("${CMAKE_SOURCE_DIR}/include/cereal" "${CMAKE_BINARY_DIR}/vendor/cereal" EXCLUDE_FROM_ALL)
    add_library(ChainedEngine::External::Cereal ALIAS cereal)
else()
    message(FATAL_ERROR "cereal submodule missing at ${CMAKE_SOURCE_DIR}/include/cereal")
endif()

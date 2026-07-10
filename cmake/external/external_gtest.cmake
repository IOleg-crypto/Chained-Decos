# GoogleTest dependency
if( EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/googletest" "${CMAKE_BINARY_DIR}/vendor/googletest" EXCLUDE_FROM_ALL)
endif()

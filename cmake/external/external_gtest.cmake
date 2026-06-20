# GoogleTest dependency
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/googletest" "${CMAKE_BINARY_DIR}/vendor/googletest" EXCLUDE_FROM_ALL)
endif()

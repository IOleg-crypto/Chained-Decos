# yaml-cpp dependency
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/yaml-cpp/CMakeLists.txt")
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(YAML_MSVC_SHARED_RT OFF CACHE BOOL "" FORCE)
    
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/yaml-cpp" "${CMAKE_BINARY_DIR}/vendor/yaml-cpp" EXCLUDE_FROM_ALL)

    # yaml-cpp 0.8.0's emitterutils.cpp uses uint16_t without including <cstdint>.
    # It compiled only because older standard libraries leaked <cstdint> transitively.
    # MinGW's libstdc++ (shared by BOTH Windows Clang and Windows GCC) no longer does,
    # so those two toolchains fail with "'uint16_t' was not declared in this scope"
    # while Linux and MSVC still build. Force-include the header for GCC/Clang instead
    # of patching the submodule source (which would revert on submodule update). MSVC
    # neither needs this nor understands -include, so it is excluded.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(yaml-cpp PRIVATE -include cstdint)
    endif()
else()
    message(FATAL_ERROR "yaml-cpp submodule missing at ${CMAKE_SOURCE_DIR}/thirdparty/yaml-cpp")
endif()

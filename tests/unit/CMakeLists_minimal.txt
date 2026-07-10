# Minimal test — just GTest, no engine at all
add_executable(engine_tests_minimal
    minimal_test.cpp
)

target_link_libraries(engine_tests_minimal PRIVATE
    gtest
    gtest_main
)

target_include_directories(engine_tests_minimal PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty/googletest/googletest/include
)

include(GoogleTest)
gtest_discover_tests(engine_tests_minimal
    CONFIGURATION_VAR CMAKE_BUILD_TYPE
    DISCOVERY_MODE POST_BUILD
    DISCOVERY_TIMEOUT 30
    PROPERTIES LABELS "Minimal"
)
add_executable(EngineTests
    scene_tests.cpp
    physics_tests.cpp
    hierarchy_tests.cpp
    event_input_tests.cpp
    #asset_manager_tests.cpp
    integration_tests.cpp
)

target_link_libraries(EngineTests PRIVATE engine editor_core GTest::gmock_main GTest::gtest_main)
target_include_directories(EngineTests PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/editor
)

if(DEFINED ENV{CI})
    target_compile_definitions(EngineTests PRIVATE CH_CI)
endif()

include(GoogleTest)
gtest_discover_tests(EngineTests 
    CONFIGURATION_VAR CMAKE_BUILD_TYPE
    DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Installation
install(TARGETS EngineTests
    RUNTIME DESTINATION bin
)

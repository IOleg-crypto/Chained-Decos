add_executable(EngineTests
    scene_tests.cpp
    physics_tests.cpp
    asset_manager_tests.cpp
)

target_link_libraries(EngineTests PRIVATE engine GTest::gmock_main GTest::gtest_main)
target_include_directories(EngineTests PRIVATE ${CMAKE_SOURCE_DIR})

add_test(NAME EngineTests COMMAND EngineTests)

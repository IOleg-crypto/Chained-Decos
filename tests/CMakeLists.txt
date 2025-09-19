enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests
        gtest.cpp
)

target_include_directories(unit_tests PRIVATE
        ../src
)

target_link_libraries(unit_tests
        PRIVATE
        ChainedDecosEngine
        player
        model
        menu
        input
        mapmod
        cameracontroller
        color
        world
        collision
        render
        raylib
        nlohmann_json
        imguilib
        GTest::gtest_main
)


add_test(NAME ColorParserTests COMMAND unit_tests --gtest_filter="ColorParserTest*")
add_test(NAME InputManagerTests COMMAND unit_tests --gtest_filter="InputManagerTest*")
add_test(NAME MenuTests COMMAND unit_tests --gtest_filter="MenuTest*")
add_test(NAME MapLoaderTests COMMAND unit_tests --gtest_filter="MapLoaderTest*")
add_test(NAME CameraControllerTests COMMAND unit_tests --gtest_filter="CameraControllerTest*")
add_test(NAME ModelInstanceTests COMMAND unit_tests --gtest_filter="ModelInstanceTest*")
add_test(NAME PlayerTests COMMAND unit_tests --gtest_filter="PlayerTest*")
add_test(NAME ModelsTests COMMAND unit_tests --gtest_filter="ModelsTest*")
add_test(NAME EngineTests COMMAND unit_tests --gtest_filter="EngineTest*")
add_test(NAME IntegrationTests COMMAND unit_tests --gtest_filter="IntegrationTest*")
add_test(NAME PerformanceTests COMMAND unit_tests --gtest_filter="PerformanceTest*")
add_test(NAME EdgeCaseTests COMMAND unit_tests --gtest_filter="EdgeCaseTest*")
add_test(NAME StressTests COMMAND unit_tests --gtest_filter="StressTest*")

add_test(NAME AllTests COMMAND unit_tests)

include(GoogleTest)
gtest_discover_tests(unit_tests)

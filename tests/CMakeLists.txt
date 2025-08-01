enable_testing()

add_executable(unit_tests
        gtest.cpp
        ../src/Player/Player.h
        ../src/Player/Player.cpp
        ../src/Model/Model.h
        ../src/Model/Model.cpp
        ../src/Model/ModelInstance.h
        ../src/Model/ModelInstance.cpp
        ../src/Menu/Menu.h
        ../src/Menu/Menu.cpp
        ../src/Engine/Engine.h
        ../src/Engine/Engine.cpp
        ../src/Map/MapLoader.h
        ../src/Map/MapLoader.cpp
        ../src/CameraController/CameraController.h
        ../src/CameraController/CameraController.cpp
        ../src/Input/InputManager.h
        ../src/Input/InputManager.cpp
)

target_include_directories(unit_tests PRIVATE
        ../src
)

target_link_libraries(unit_tests
        PRIVATE
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

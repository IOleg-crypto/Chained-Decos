enable_testing()

add_executable(unit_tests
        gtest.cpp
        ../src/Player/Player.cpp
        ../src/Model/Model.cpp
        ../src/Model/ModelInstance.cpp
        ../src/Menu/Menu.cpp
        ../src/Engine/Engine.cpp
        ../src/Map/MapLoader.cpp
        ../src/CameraController/CameraController.cpp
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
add_test(unit_tests HelloTest)

include(GoogleTest)
gtest_discover_tests(unit_tests)

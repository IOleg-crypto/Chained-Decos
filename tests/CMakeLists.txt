# 1. Оголошуємо саме ВИКОНУВАНИЙ ФАЙЛ (програму)
add_executable(engine_tests
    uuid_tests.cpp
    primitive_tests.cpp
    serialization_tests.cpp
    scriptengine_tests.cpp
    scene_tests.cpp
    physics_tests.cpp
    hierarchy_tests.cpp
    event_input_tests.cpp
    asset_manager_tests.cpp
    integration_tests.cpp
    importer_tests.cpp
    renderer_tests.cpp
    scene_serialization_tests.cpp
    material_tests.cpp
    audio_tests.cpp
    test_environment.cpp
)

target_link_libraries(engine_tests PRIVATE
    engine_reflection
    engine_core
    engine_app
    engine_scene
    engine_graphics
    engine_project
    engine_physics
    engine_audio
    engine_scripting
    engine_external_entt
    glm
    engine_external_imgui
    gtest_main  
)

# 3. Шляхи до інклудів
target_include_directories(engine_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(engine_tests 
    CONFIGURATION_VAR CMAKE_BUILD_TYPE
    DISCOVERY_MODE POST_BUILD   
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    PROPERTIES LABELS "Engine;Core"
)
install(TARGETS engine_tests
    RUNTIME DESTINATION bin
)
# Unit tests — fast, no full engine runtime required
# Links against specific engine modules, not the full engine facade.

set(UNIT_SOURCES
    test_main_unit.cpp
    uuid_tests.cpp
    timestep_tests.cpp
    color_tests.cpp
    thread_pool_tests.cpp
    event_tests.cpp
    event_input_tests.cpp
    layer_stack_tests.cpp
    service_locator_tests.cpp
)

add_executable(engine_tests_unit ${UNIT_SOURCES})

target_link_libraries(engine_tests_unit PRIVATE
    engine_common
    engine_core
    gtest
    gtest_main
)

# MinGW/gcc: statically link the gcc/stdc++ runtime so the test exe can start
# during test discovery without libstdc++-6.dll / libgcc_s / libwinpthread on PATH.
if(MINGW)
    target_link_options(engine_tests_unit PRIVATE -static-libgcc -static-libstdc++)
endif()

target_include_directories(engine_tests_unit PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Engine PCH is PRIVATE on engine modules; apply it here explicitly.
if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_tests_unit)
endif()

# Unit tests run as a SINGLE ctest entry (one process for the whole binary)
# rather than via gtest_discover_tests. PRE_TEST discovery generates per-test
# include files whose names contain characters (e.g. the [1] index suffix)
# that break path resolution on Windows, producing
# "engine_tests_unit[1]_include-.cmake ... could not be found" CTest errors.
# A single entry keeps gtest's own per-test reporting and readable output.
add_test(NAME UnitSuite COMMAND engine_tests_unit)
set_tests_properties(UnitSuite PROPERTIES
    LABELS "Unit"
    ENVIRONMENT "ENGINE_ROOT=${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    TIMEOUT 120
)

add_library(engine_audio ${SUBMODULE_LIB_TYPE}
        audio.cpp
        audio.h
)

target_include_directories(engine_audio PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/entt/src
    ${CMAKE_SOURCE_DIR}/include/miniaudio
)


target_link_libraries(engine_audio PUBLIC 
    engine_core
)

if(WIN32)
    target_link_libraries(engine_audio PUBLIC winmm ole32)
endif()

if(MSVC)
    target_compile_options(engine_audio PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_audio PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_audio)
endif()

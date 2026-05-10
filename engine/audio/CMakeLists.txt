add_library(engine_audio ${SUBMODULE_LIB_TYPE}
        audio.cpp
        audio.h
)

target_include_directories(engine_audio PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(engine_audio 
PUBLIC 
    engine_core
    ChainedEngine::External::GLM
    ChainedEngine::External::Miniaudio
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

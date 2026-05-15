chained_add_engine_module(engine_audio
    SOURCES
        audio.cpp
        audio.h
    DEPENDS
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

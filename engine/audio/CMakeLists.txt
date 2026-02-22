add_library(engine_audio OBJECT
    audio.cpp
    sound_asset.cpp
    audio_importer.cpp

    audio.h
    sound_asset.h
    audio_importer.h
)

target_include_directories(engine_audio PUBLIC
    ${ENGINE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/entt/src
)

target_link_libraries(engine_audio PUBLIC 
    raylib 
)

if(MSVC)
    target_compile_options(engine_audio PRIVATE /bigobj)
elseif(MINGW)
    target_compile_options(engine_audio PRIVATE -Wa,-mbig-obj)
endif()

if(COMMAND apply_engine_optimizations)
    apply_engine_optimizations(engine_audio)
endif()

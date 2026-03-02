#include "engine/audio/audio.h"
#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "gtest/gtest.h"


using namespace CHEngine;

TEST(AudioTest, InitializationAndShutdown)
{
    // Audio is already initialized by TestEnvironment/Application
    EXPECT_NO_THROW(Audio::Get());
}

TEST(AudioTest, PlayWithPath)
{
    Audio& audio = Audio::Get();

    // This should not crash even if the file doesn't exist (it will just warn/log)
    EXPECT_NO_THROW({
        audio.Play("non_existent_sound.wav");
        audio.Stop("non_existent_sound.wav");
    });
}

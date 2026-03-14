#include "engine/audio/audio.h"
#include "gtest/gtest.h"

using namespace CHEngine;

TEST(AudioTest, InitializationAndShutdown)
{
    auto& audio = Audio::Get();
    // Test that we can call Init/Shutdown safely even if already initialized
    audio.Init();
    audio.Shutdown();
}

TEST(AudioTest, PlayWithoutAsset)
{
    auto& audio = Audio::Get();
    audio.Init();

    // Trying to play a null asset should safely be ignored and not crash
    std::shared_ptr<SoundAsset> nullAsset = nullptr;
    
    EXPECT_NO_THROW({
        audio.Play(nullAsset);
        audio.Stop(nullAsset);
    });

    audio.Shutdown();
}

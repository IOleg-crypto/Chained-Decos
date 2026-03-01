#include "engine/audio/audio.h"
#include "gtest/gtest.h"

using namespace CHEngine;

TEST(AudioTest, InitializationAndShutdown)
{
    Audio audio;
    // Initialize audio device
    audio.Init();
    
    // We expect it to be safe to call shutdown
    audio.Shutdown();
    
    // Test multiple init/shutdowns to ensure no state leaking or crashes
    audio.Init();
    audio.Shutdown();
}

TEST(AudioTest, PlayWithoutAsset)
{
    Audio audio;
    audio.Init();

    // Trying to play a null asset should safely be ignored and not crash
    std::shared_ptr<SoundAsset> nullAsset = nullptr;
    
    EXPECT_NO_THROW({
        audio.Play(nullAsset);
        audio.Stop(nullAsset);
    });

    audio.Shutdown();
}

#include "engine/audio/audio.h"
#include "gtest/gtest.h"

using namespace CHEngine;

TEST(AudioTest, InitializationAndShutdown)
{
    // Initialize audio device
    Audio::Init();
    
    // We expect it to be safe to call shutdown
    Audio::Shutdown();
    
    // Test multiple init/shutdowns to ensure no state leaking or crashes
    Audio::Init();
    Audio::Shutdown();
}

TEST(AudioTest, PlayWithoutAsset)
{
    Audio::Init();

    // Trying to play a null asset should safely be ignored and not crash
    std::shared_ptr<SoundAsset> nullAsset = nullptr;
    
    EXPECT_NO_THROW({
        Audio::Play(nullAsset);
        Audio::Stop(nullAsset);
    });

    Audio::Shutdown();
}

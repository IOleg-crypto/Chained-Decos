#include "engine/audio/audio.h"
#include "engine/assets/loaders/audio_loader.h"
#include "gtest/gtest.h"

using namespace Chained;

TEST(AudioTest, EmptyBufferPlaybackIsNoOp)
{
    Audio audio;

    EXPECT_NO_THROW(audio.Play(AudioHandle(0)));
    EXPECT_NO_THROW(audio.Update(Timestep(0.016f)));
    EXPECT_NO_THROW(audio.StopAll());
}

TEST(AudioTest, TinyBufferPlaybackAndStopAllAreSafe)
{
    Audio audio;

    EXPECT_NO_THROW(audio.Play(AudioHandle(0), 0.25f, 1.0f, false, false));
    EXPECT_NO_THROW(audio.Update(Timestep(0.016f)));
    EXPECT_NO_THROW(audio.StopAll());
}

TEST(AudioTest, LoadAudio)
{
    auto audio = AudioLoader::Create();
    AudioLoader::Load(audio, "resources/audio/default.wav");
    EXPECT_EQ(audio->GetType(), AssetType::Audio);

}

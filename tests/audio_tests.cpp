#include "engine/audio/audio.h"
#include "gtest/gtest.h"

using namespace CHEngine;

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

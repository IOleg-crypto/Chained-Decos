#include "engine/audio/audio.h"
#include "gtest/gtest.h"

#include <array>

using namespace CHEngine;

TEST(AudioTest, SingletonReturnsSameInstance)
{
    auto& first = Audio::Get();
    auto& second = Audio::Get();
    EXPECT_EQ(&first, &second);
}

TEST(AudioTest, EmptyBufferPlaybackIsNoOp)
{
    auto& audio = Audio::Get();

    EXPECT_NO_THROW(audio.Play(AudioHandle(0)));
    EXPECT_NO_THROW(audio.Update(Timestep(0.016f)));
    EXPECT_NO_THROW(audio.StopAll());
}

TEST(AudioTest, TinyBufferPlaybackAndStopAllAreSafe)
{
    auto& audio = Audio::Get();

    EXPECT_NO_THROW(audio.Play(AudioHandle(0), 0.25f, 1.0f, false, false));
    EXPECT_NO_THROW(audio.Update(Timestep(0.016f)));
    EXPECT_NO_THROW(audio.StopAll());
}

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
    AudioBuffer empty{};

    EXPECT_NO_THROW(audio.Play(empty));
    EXPECT_NO_THROW(audio.Update(Timestep(0.016f)));
    EXPECT_NO_THROW(audio.StopAll());
}

TEST(AudioTest, TinyBufferPlaybackAndStopAllAreSafe)
{
    auto& audio = Audio::Get();

    std::array<float, 64> samples{};
    AudioBuffer buffer{};
    buffer.Data = samples.data();
    buffer.Size = static_cast<uint32_t>(samples.size());
    buffer.Channels = 1;
    buffer.SampleRate = 48000;

    EXPECT_NO_THROW(audio.Play(buffer, 0.25f, 1.0f, false, false));
    EXPECT_NO_THROW(audio.Update(Timestep(0.016f)));
    EXPECT_NO_THROW(audio.StopAll());
}

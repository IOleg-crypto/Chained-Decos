#ifndef CH_AUDIO_ASSET_H
#define CH_AUDIO_ASSET_H

#include "engine/assets/asset.h"
#include <cstdint>
#include <string>

namespace Chained
{

class AudioAsset : public Asset
{
public:
    AudioAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~AudioAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Audio;
    }

    float GetDuration() const { return m_Duration; }
    uint32_t GetSampleRate() const { return m_SampleRate; }
    uint32_t GetChannels() const { return m_Channels; }
    uint64_t GetFrameCount() const { return m_FrameCount; }

    void SetDuration(float duration) { m_Duration = duration; }
    void SetSampleRate(uint32_t sampleRate) { m_SampleRate = sampleRate; }
    void SetChannels(uint32_t channels) { m_Channels = channels; }
    void SetFrameCount(uint64_t frameCount) { m_FrameCount = frameCount; }

private:
    float m_Duration = 0.0f;
    uint32_t m_SampleRate = 0;
    uint32_t m_Channels = 0;
    uint64_t m_FrameCount = 0;
};

} // namespace Chained

#endif // CH_AUDIO_ASSET_H

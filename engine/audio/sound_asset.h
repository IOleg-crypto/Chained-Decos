#ifndef CH_SOUND_ASSET_H
#define CH_SOUND_ASSET_H

#include "engine/core/assets/asset.h"
#include "engine/audio/audio.h"
#include <vector>
#include <cstdint>

namespace CHEngine
{

class SoundAsset : public Asset
{
public:
    SoundAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~SoundAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Audio;
    }

    void OnLoaded() override {}

    const std::vector<float>& GetPCMData() const { return m_PCMData; }
    uint32_t GetChannels() const { return m_Channels; }
    uint32_t GetSampleRate() const { return m_SampleRate; }

    void SetPCMData(std::vector<float>&& pcmData) { m_PCMData = std::move(pcmData); }
    void SetChannels(uint32_t channels) { m_Channels = channels; }
    void SetSampleRate(uint32_t sampleRate) { m_SampleRate = sampleRate; }

    AudioBuffer GetBuffer() const
    {
        AudioBuffer buffer;
        buffer.Data = m_PCMData.data();
        buffer.Size = (uint32_t)m_PCMData.size();
        buffer.Channels = m_Channels;
        buffer.SampleRate = m_SampleRate;
        return buffer;
    }

private:
    std::vector<float> m_PCMData;
    uint32_t m_Channels = 0;
    uint32_t m_SampleRate = 0;
};

} // namespace CHEngine

#endif // CH_SOUND_ASSET_H

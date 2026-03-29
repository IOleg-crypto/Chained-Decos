#include "engine/audio/loaders/audio_loader.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/log.h"
#include "miniaudio.h"
#include <filesystem>

namespace CHEngine
{
    std::shared_ptr<Asset> AudioLoader::Create()
    {
        return std::make_shared<SoundAsset>();
    }

    bool AudioLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath)
    {
        auto soundAsset = std::static_pointer_cast<SoundAsset>(asset);
        
        if (resolvedPath.empty()) return false;

        std::filesystem::path fullPath(resolvedPath);
        if (!std::filesystem::exists(fullPath))
        {
            CH_CORE_ERROR("AudioLoader: File not found: {}", resolvedPath);
            return false;
        }

        ma_decoder decoder;
        ma_result result = ma_decoder_init_file(resolvedPath.c_str(), NULL, &decoder);
        if (result != MA_SUCCESS)
        {
            CH_CORE_ERROR("AudioLoader: Failed to initialize decoder for {}", resolvedPath);
            return false;
        }

        soundAsset->SetChannels(decoder.outputChannels);
        soundAsset->SetSampleRate(decoder.outputSampleRate);

        ma_uint64 frameCount;
        result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
        if (result != MA_SUCCESS)
        {
            CH_CORE_ERROR("AudioLoader: Failed to get length for {}", resolvedPath);
            ma_decoder_uninit(&decoder);
            return false;
        }

        // Allocate memory and read frames
        std::vector<float> pcmData(frameCount * decoder.outputChannels);
        ma_uint64 framesRead;
        result = ma_decoder_read_pcm_frames(&decoder, pcmData.data(), frameCount, &framesRead);
        
        soundAsset->SetPCMData(std::move(pcmData));
        ma_decoder_uninit(&decoder);

        if (result != MA_SUCCESS)
        {
            CH_CORE_ERROR("AudioLoader: Failed to read PCM frames for {}", resolvedPath);
            return false;
        }

        CH_CORE_INFO("AudioLoader: Successfully decoded {} ({} frames)", resolvedPath, frameCount);
        return true;
    }
} // namespace CHEngine

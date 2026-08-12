#include "engine/assets/loaders/audio_loader.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include <miniaudio.h>
#include <filesystem>
#include <vector>

namespace Chained
{

	std::shared_ptr<Asset> AudioLoader::Create()
	{
		return std::make_shared<AudioAsset>();
	}

	bool AudioLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
	{
		auto audioAsset = std::static_pointer_cast<AudioAsset>(asset);

		ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
		ma_decoder decoder;
		ma_result result;

		// Try reading from pack first
		std::vector<uint8_t> packData;
		bool usePack = false;
		auto* am = ServiceLocator::TryGet<AssetManager>();
		if (am)
		{
			packData = am->ReadAssetData(resolvedPath);
			usePack = !packData.empty();
		}

		if (usePack)
		{
			result = ma_decoder_init_memory(packData.data(), packData.size(), &config, &decoder);
		}
		else
		{
			if (!am || !am->FileExists(resolvedPath))
			{
				if (outError)
				{
					*outError = "AudioLoader: File not found: " + resolvedPath;
				}
				return false;
			}
			result = ma_decoder_init_file(resolvedPath.c_str(), &config, &decoder);
		}

		if (result != MA_SUCCESS)
		{
			if (outError)
			{
				*outError = "AudioLoader: Failed to decode " + resolvedPath;
			}
			return false;
		}

		ma_uint64 frameCount = 0;
		ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

		ma_format format;
		ma_uint32 channels = 0;
		ma_uint32 sampleRate = 0;
		result = ma_decoder_get_data_format(&decoder, &format, &channels, &sampleRate, NULL, 0);
		if (result != MA_SUCCESS)
		{
			ma_decoder_uninit(&decoder);
			if (outError)
			{
				*outError = "AudioLoader: Failed to get data format for " + resolvedPath;
			}
			return false;
		}

		float duration = 0.0f;
		if (sampleRate > 0)
		{
			duration = static_cast<float>(frameCount) / static_cast<float>(sampleRate);
		}

		audioAsset->SetDuration(duration);
		audioAsset->SetSampleRate(sampleRate);
		audioAsset->SetChannels(channels);
		audioAsset->SetFrameCount(frameCount);

		ma_decoder_uninit(&decoder);

		CH_CORE_INFO("AudioLoader: Loaded '{}' — {} ch, {} Hz, {:.2f}s",
					 std::filesystem::path(resolvedPath).filename().string(), channels, sampleRate, duration);
		return true;
	}

} // namespace Chained

#ifndef CH_AUDIO_COMPONENT_H
#define CH_AUDIO_COMPONENT_H

#include "engine/assets/asset.h"
#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include <glm/glm.hpp>
#include <string>

namespace Chained
{
	struct AudioComponent
	{
		AssetHandle SoundHandle = AssetHandle(0);
		std::string SoundPath;
		uint64_t SoundUUID = 0;
		float Volume = 1.0f;
		float Pitch = 1.0f;
		bool Loop = false;
		bool PlayOnStart = true;
		bool Spatialized = false;
		glm::vec3 Position = {0, 0, 0};
		float MinDistance = 1.0f;
		float MaxDistance = 100.0f;

		// Runtime
		bool IsPlaying = false;

		static const char* GetStaticName()
		{
			return "AudioComponent";
		}

		struct UI
		{
			UIMeta SoundHandle = {.ReadOnly = true, .Transient = true};
			UIMeta SoundPath = {.Hint = PropertyMeta::WidgetHint::FilePicker, .Extensions = ".wav,.mp3,.ogg"};
			UIMeta SoundUUID = {.ReadOnly = true};
			UIMeta Volume = {.Min = 0.0f, .Max = 2.0f, .Speed = 0.01f, .Tooltip = "Volume of the audio source"};
			UIMeta Pitch = {
				.Min = 0.1f, .Max = 3.0f, .Speed = 0.01f, .Tooltip = "Playback speed/pitch of the audio source"};
			UIMeta MinDistance = {
				.Min = 0.0f, .Max = 500.0f, .Speed = 0.5f, .Tooltip = "Radius where the sound starts to fade"};
			UIMeta MaxDistance = {
				.Min = 1.0f, .Max = 10000.0f, .Speed = 5.0f, .Tooltip = "Maximum distance for 3D sound visibility"};
		};
	};

	CH_MARK_RFL(AudioComponent);

} // namespace Chained

#endif // CH_AUDIO_COMPONENT_H
#ifndef CH_AUDIO_IMPORTER_H
#define CH_AUDIO_IMPORTER_H

#include "engine/audio/sound_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
class AudioImporter
{
public:
    static std::shared_ptr<SoundAsset> ImportSound(const std::string& path);

    // For async loading
    static void ImportSoundAsync(const std::shared_ptr<SoundAsset>& asset, const std::string& path);
};
} // namespace CHEngine

#endif // CH_AUDIO_IMPORTER_H

#ifndef CH_AUDIO_LOADER_H
#define CH_AUDIO_LOADER_H

#include "engine/core/assets/asset_loader.h"
// #include "engine/audio/sound_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
class AudioLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override;
    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override;
    bool IsAsync() const override { return true; }
};
} // namespace CHEngine

#endif // CH_AUDIO_LOADER_H

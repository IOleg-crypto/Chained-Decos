#ifndef CH_AUDIO_LOADER_H
#define CH_AUDIO_LOADER_H

#include "engine/assets/types/audio_asset.h"
#include "engine/assets/loaders/asset_loader.h"
#include <memory>
#include <string>

namespace Chained
{
namespace AudioLoader
{
std::shared_ptr<Asset> Create();
bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);
} // namespace AudioLoader
} // namespace Chained

#endif // CH_AUDIO_LOADER_H

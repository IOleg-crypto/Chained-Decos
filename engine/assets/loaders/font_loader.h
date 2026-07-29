#ifndef CH_FONT_LOADER_H
#define CH_FONT_LOADER_H

#include "engine/assets/asset.h"
#include <memory>
#include <string>

namespace Chained
{
namespace FontLoader
{
std::shared_ptr<Asset> Create();
bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError = nullptr);
} // namespace FontLoader
} // namespace Chained

#endif // CH_FONT_LOADER_H
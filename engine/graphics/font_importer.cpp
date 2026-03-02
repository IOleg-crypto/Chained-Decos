#include "font_importer.h"
#include "engine/core/log.h"
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<FontAsset> FontImporter::ImportFont(const std::string& path)
{
    if (path.empty())
    {
        return nullptr;
    }

    auto asset = std::make_shared<FontAsset>();
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);

    return asset;
}
} // namespace CHEngine

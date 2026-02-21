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

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("FontImporter: File not found: {}", path);
        return nullptr;
    }

    Font font = ::LoadFont(fullPath.string().c_str());
    if (font.texture.id == 0)
    {
        CH_CORE_ERROR("FontImporter: Failed to load font: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<FontAsset>();
    asset->SetPath(path);
    asset->GetFont() = font;
    asset->SetState(AssetState::Ready);

    return asset;
}
} // namespace CHEngine

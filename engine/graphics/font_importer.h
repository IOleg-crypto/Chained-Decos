#ifndef CH_FONT_IMPORTER_H
#define CH_FONT_IMPORTER_H

#include "engine/graphics/font_asset.h"
#include <memory>
#include <string>

namespace CHEngine
{
class FontImporter
{
public:
    static std::shared_ptr<FontAsset> ImportFont(const std::string& path);
};
} // namespace CHEngine

#endif // CH_FONT_IMPORTER_H

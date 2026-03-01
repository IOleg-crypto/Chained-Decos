#ifndef CH_ASSET_IMPORTER_H
#define CH_ASSET_IMPORTER_H

#include "asset.h"
#include <filesystem>
#include <memory>

namespace CHEngine
{
class AssetImporter
{
public:
    virtual ~AssetImporter() = default;
};
} // namespace CHEngine

#endif // CH_ASSET_IMPORTER_H

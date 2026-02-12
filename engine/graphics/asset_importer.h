#ifndef CH_ASSET_IMPORTER_H
#define CH_ASSET_IMPORTER_H

#include "asset.h"
#include <memory>
#include <filesystem>

namespace CHEngine
{
    class AssetImporter
    {
    public:
        virtual ~AssetImporter() = default;
    };
}

#endif // CH_ASSET_IMPORTER_H

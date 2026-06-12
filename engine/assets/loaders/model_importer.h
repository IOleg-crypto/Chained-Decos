#ifndef CH_MODEL_IMPORTER_H
#define CH_MODEL_IMPORTER_H

#include "engine/assets/asset_metadata.h"
#include "engine/assets/types/model_asset.h"
#include <memory>

namespace Chained
{
class ModelImporter
{
public:
    static std::shared_ptr<ModelAsset> ImportModel(AssetHandle handle, const AssetMetadata& metadata);
};
} // namespace Chained

#endif // CH_MODEL_IMPORTER_H

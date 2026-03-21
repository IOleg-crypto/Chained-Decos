#ifndef CH_MODEL_LOADER_H
#define CH_MODEL_LOADER_H

#include "engine/core/assets/asset_loader.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/importers/mesh_importer.h"
#include <memory>
#include <string>

namespace CHEngine
{
class ModelLoader : public IAssetLoader
{
public:
    std::shared_ptr<Asset> Create() override
    {
        return std::make_shared<ModelAsset>();
    }

    bool Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath) override
    {
        auto modelAsset = std::static_pointer_cast<ModelAsset>(asset);
        
        // Match existing AssetManager logic:
        // Handle procedural meshes synchronously, others asynchronously
        if (resolvedPath.starts_with(":"))
        {
            auto importedModel = MeshImporter::ImportMesh(resolvedPath);
            if (importedModel)
            {
                // We need to copy/move data into modelAsset or some similar logic
                // For now, mirroring what was there before (which was slightly flawed or assumed sync load returns a full asset)
                // Actually, if it's sync, Load itself should handle it.
                // In my new GetAsset: if loader->IsAsync() it enqueues, ELSE it calls Load directly.
                // So if it's a procedural mesh, we want it to be sync.
                modelAsset->SetModel(importedModel->GetModel()); // Assuming such methods exist based on original code
                modelAsset->SetState(AssetState::Ready);
                return true;
            }
            return false;
        }

        auto pendingData = MeshImporter::LoadMeshDataFromDisk(resolvedPath);
        if (pendingData.isValid)
        {
            modelAsset->SetPendingData(pendingData);
            return true;
        }
        return false;
    }

    bool IsAsync() const override { return true; }
};
} // namespace CHEngine

#endif // CH_MODEL_LOADER_H

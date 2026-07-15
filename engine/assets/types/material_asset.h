#ifndef CH_MATERIAL_ASSET_H
#define CH_MATERIAL_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/renderer_types.h"
#include <string>

namespace Chained
{

class MaterialAsset : public Asset
{
public:
    MaterialAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~MaterialAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Material;
    }

    Material& GetMaterial() { return m_Material; }
    const Material& GetMaterial() const { return m_Material; }

    void SetMaterial(const Material& mat) { m_Material = mat; }

    void SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path, std::string* outError = nullptr);

private:
    Material m_Material;
};

} // namespace Chained

#endif // CH_MATERIAL_ASSET_H

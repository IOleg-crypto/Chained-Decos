#ifndef CH_MATERIAL_PANEL_H
#define CH_MATERIAL_PANEL_H

#include "engine/scene/components/mesh_component.h"
#include "engine/scene/entity.h"
#include "panel.h"


namespace Chained
{
class MaterialPanel : public Panel
{
public:
    MaterialPanel();
    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnEvent(Event& e) override;
    virtual void SetContext(const std::shared_ptr<Scene>& context) override;

private:
    void DrawMaterialSettings(Material& material);
    uint32_t GetTextureID(AssetHandle handle);

private:
    Entity m_SelectedEntity;
    int m_SelectedMeshIndex = -1;
    int m_SelectedMaterialIndex = 0;
};
} // namespace Chained

#endif // CH_MATERIAL_PANEL_H

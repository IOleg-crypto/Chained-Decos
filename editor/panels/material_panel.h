#ifndef CH_MATERIAL_PANEL_H
#define CH_MATERIAL_PANEL_H

#include "panel.h"
#include "engine/scene/entity.h"
#include "engine/scene/components/mesh_component.h"

namespace CHEngine
{
class MaterialPanel : public Panel
{
public:
    MaterialPanel();
    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnEvent(Event& e) override;
    virtual void SetContext(const std::shared_ptr<Scene>& context) override;

private:
    void DrawMaterialSlot(MaterialSlot& slot);

private:
    Entity m_SelectedEntity;
    int m_SelectedMeshIndex = -1;
    int m_SelectedMaterialIndex = 0;
};
} // namespace CHEngine

#endif // CH_MATERIAL_PANEL_H

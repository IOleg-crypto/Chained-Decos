#ifndef CH_MATERIAL_PANEL_H
#define CH_MATERIAL_PANEL_H

#include "panel.h"
#include "engine/scene/entity.h"

namespace CHEngine
{
class MaterialPanel : public Panel
{
public:
    MaterialPanel();
    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnEvent(Event& e) override;

private:
    Entity m_SelectedEntity;
    int m_SelectedMeshIndex = -1;
};
} // namespace CHEngine

#endif // CH_MATERIAL_PANEL_H

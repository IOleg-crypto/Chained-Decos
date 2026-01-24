#ifndef CH_PROJECT_SETTINGS_PANEL_H
#define CH_PROJECT_SETTINGS_PANEL_H

#include "panel.h"

namespace CHEngine
{
class ProjectSettingsPanel : public Panel
{
public:
    ProjectSettingsPanel();
    virtual void OnImGuiRender(bool readOnly = false) override;
};
} // namespace CHEngine

#endif // CH_PROJECT_SETTINGS_PANEL_H

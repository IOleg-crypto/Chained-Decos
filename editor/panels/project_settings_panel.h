#ifndef CH_PROJECT_SETTINGS_PANEL_H
#define CH_PROJECT_SETTINGS_PANEL_H

#include "panel.h"

namespace Chained
{
class ProjectSettingsPanel : public Panel
{
public:
    ProjectSettingsPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;
};
} // namespace Chained

#endif // CH_PROJECT_SETTINGS_PANEL_H

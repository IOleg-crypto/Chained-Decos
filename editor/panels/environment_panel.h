#ifndef CH_ENVIRONMENT_PANEL_H
#define CH_ENVIRONMENT_PANEL_H

#include "panel.h"

namespace CHEngine
{
class EnvironmentPanel : public Panel
{
public:
    EnvironmentPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;

private:
    void DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly);
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_PANEL_H

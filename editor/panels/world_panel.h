#ifndef CH_WORLD_PANEL_H
#define CH_WORLD_PANEL_H

#include "panel.h"

namespace Chained
{
class WorldPanel : public Panel
{
public:
    WorldPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;

private:
    void DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly);
};
} // namespace Chained

#endif // CH_WORLD_PANEL_H

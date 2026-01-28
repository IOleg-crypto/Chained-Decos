#ifndef CH_ENVIRONMENT_PANEL_H
#define CH_ENVIRONMENT_PANEL_H

// Removed redundant include: engine/graphics/render.h
#include "panel.h"

namespace CHEngine
{
class EnvironmentPanel : public Panel
{
public:
    EnvironmentPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;

public:
    void SetDebugFlags(DebugRenderFlags *flags)
    {
        m_DebugFlags = flags;
    }

private:
    void DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly);

private:
    DebugRenderFlags *m_DebugFlags = nullptr;
};
} // namespace CHEngine

#endif // CH_ENVIRONMENT_PANEL_H

#ifndef CH_PROFILER_PANEL_H
#define CH_PROFILER_PANEL_H

#include "panel.h"

namespace CHEngine
{
class ProfilerPanel : public Panel
{
public:
    ProfilerPanel();
    virtual void OnImGuiRender(bool readOnly = false) override;

private:
    void DrawProfileResult(const struct ProfileResult &result);
};
} // namespace CHEngine

#endif // CH_PROFILER_PANEL_H

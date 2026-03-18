#ifndef CH_EFFECTS_PANEL_H
#define CH_EFFECTS_PANEL_H

#include "panel.h"

namespace CHEngine
{
class EffectsPanel : public Panel
{
public:
    EffectsPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;
};
} // namespace CHEngine

#endif // CH_EFFECTS_PANEL_H

#ifndef CH_IMGUI_LAYER_H
#define CH_IMGUI_LAYER_H

#include "engine/core/layer.h"

namespace CHEngine
{
class ImGuiLayer : public Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnEvent(Event& e) override;

    void Begin();
    void End();

    void BlockEvents(bool block)
    {
        m_BlockEvents = block;
    }

private:
    bool m_BlockEvents = true;
};
} // namespace CHEngine

#endif // CH_IMGUI_LAYER_H

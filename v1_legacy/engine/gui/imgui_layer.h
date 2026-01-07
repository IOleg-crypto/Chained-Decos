#ifndef CD_ENGINE_GUI_IMGUI_LAYER_H
#define CD_ENGINE_GUI_IMGUI_LAYER_H

#include "engine/core/layer/layer.h"

namespace CHEngine
{
class ImGuiLayer : public Layer
{
public:
    ImGuiLayer();
    ~ImGuiLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnEvent(Event &e) override;

    void Begin();
    void End();

private:
    float m_Time = 0.0f;
};
} // namespace CHEngine

#endif

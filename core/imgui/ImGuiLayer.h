#ifndef IMGUI_LAYER_H
#define IMGUI_LAYER_H

#include "core/layer/Layer.h"

namespace CHEngine
{

/**
 * ImGuiLayer - Manages ImGui context and rendering
 */
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

#endif // IMGUI_LAYER_H

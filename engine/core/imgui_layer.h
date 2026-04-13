#ifndef CH_IMGUI_LAYER_H
#define CH_IMGUI_LAYER_H

#include "engine/core/layer.h"
#include <imgui.h>

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

    // Recreate renderer device objects so newly added ImGui fonts become visible.
    bool RefreshFontAtlasTexture();

    void BlockEvents(bool block)
    {
        m_BlockEvents = block;
    }

    static void SetContext(ImGuiContext* context);
    ImGuiContext* GetContext() const { return ImGui::GetCurrentContext(); }

private:
    bool m_BlockEvents = true;
};
} // namespace CHEngine

#endif // CH_IMGUI_LAYER_H

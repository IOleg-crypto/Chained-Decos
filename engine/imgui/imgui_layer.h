#ifndef CH_IMGUI_LAYER_H
#define CH_IMGUI_LAYER_H

#include "engine/core/layer.h"
#include <imgui.h>

namespace Chained
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

    // Centralized font loading in the DLL to avoid cross-module atlas corruption
    void* AddFontFromFile(const std::string& path, float size, const void* config = nullptr, const void* ranges = nullptr);

    void BlockEvents(bool block)
    {
        m_BlockEvents = block;
    }

    static void SetContext(ImGuiContext* context);
    void* GetContext() const { return ImGui::GetCurrentContext(); }

private:
    bool m_BlockEvents = true;
};
} // namespace Chained

#endif // CH_IMGUI_LAYER_H

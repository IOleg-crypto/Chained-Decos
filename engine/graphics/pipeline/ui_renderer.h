#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

#include "ui_font_registry.h"
#include "engine/scene/scene.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "entt/entt.hpp"
#include "imgui.h"
#include <map>
#include <vector>
#include <string>

namespace CHEngine
{
class Scene;
class Entity;

// Helpers moved into UIRenderer

// Replaces raylib's Rectangle to avoid including raylib.h
struct UIRect {
    float x;
    float y;
    float width;
    float height;
};

// --- Style helpers ----------------------------------------------------------

inline ImVec4 ToImVec4(Color c)
{
    return {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
}

class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    static void Init();
    static void Shutdown();
    static UIRenderer& Get();

    // Main entry point for drawing UI for a scene.
    void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

    // --- Font Support ---
    // Call between rlImGuiBeginInitImGui() and rlImGuiEndInitImGui() at project load.
    // Scans <project>/assets/fonts/ for all TTF/OTF files.
    void LoadProjectFonts();

    // Editor requires standalone rectangle calculation
    UIRect GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos);

    // Access the font registry (e.g. to query loaded fonts).
    UIFontRegistry& GetFontRegistry() { return m_FontRegistry; }
    const UIFontRegistry& GetFontRegistry() const { return m_FontRegistry; }

private:
    bool RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode);
    std::vector<entt::entity> SortUIEntities(entt::registry& registry);
    UIRect CalculateEntityRect(Entity entity, const UIRect& canvasRect, std::map<entt::entity, UIRect>& rectCache);
private:
    struct StyleCounts
    {
        int  colors   = 0;
        int  vars     = 0;
        int  fonts    = 0;
        bool disabled = false;
    };

    void PopUIStyle(const StyleCounts& c);
    void PushTextStyle(const TextStyle& text, StyleCounts& c);
    StyleCounts PushUIStyle(const UIStyle& style, bool interactable = true);

    void RenderPanel(const PanelControl& panel, const ImVec2& pos, const ImVec2& size);
    void RenderLabel(const LabelControl& label, const ImVec2& size);
    void RenderButton(Entity entity, ButtonControl& button, const ImVec2& size, bool& itemHandled);
    void RenderSlider(SliderControl& slider, const ImVec2& size, bool& itemHandled);
    void RenderCheckbox(CheckboxControl& cb, bool& itemHandled);
    void RenderImage(const ImageControl& image, const ImVec2& size);
    void RenderInputText(Entity entity, InputTextControl& it, const ImVec2& size, bool& itemHandled);
    void RenderProgressBar(const ProgressBarControl& pb, const ImVec2& size);
    void RenderComboBox(ComboBoxControl& cb, const ImVec2& size, bool& itemHandled);
    void RenderImageButton(ImageButtonControl& ib, const ImVec2& size, bool& itemHandled);
    void RenderRadioButton(RadioButtonControl& rb, bool& itemHandled);
    void RenderColorPicker(ColorPickerControl& cp, bool& itemHandled);
    void RenderSeparator(const SeparatorControl& sep);
    void RenderDragFloat(DragFloatControl& df, const ImVec2& size, bool& itemHandled);
    void RenderDragInt(DragIntControl& di, const ImVec2& size, bool& itemHandled);
    void RenderTreeNode(TreeNodeControl& tn, bool& itemHandled);
    void RenderCollapsingHeader(CollapsingHeaderControl& ch, bool& itemHandled);
    void RenderPlotLines(const PlotLinesControl& pl, bool& itemHandled);
    void RenderPlotHistogram(const PlotHistogramControl& ph, bool& itemHandled);
    void RenderTabBar(Entity tabBarEntity, const TabBarControl& tb, entt::registry& registry);

private:
    UIFontRegistry m_FontRegistry;
    static UIRenderer* s_Instance;
};

} // namespace CHEngine

#endif // CH_UI_RENDERER_H

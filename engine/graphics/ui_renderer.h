#ifndef CH_UI_RENDERER_H
#define CH_UI_RENDERER_H

#include "engine/scene/components/control_component.h"
#include "engine/scene/scene.h"
#include "imgui.h"

namespace CHEngine
{
class Renderer;
struct UIRendererData
{
    // Internal state like input buffers could go here
    std::map<entt::entity, std::vector<char>> InputBuffers;
};

class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    void Init();
    void Shutdown();

    // Main entry point for drawing UI for a scene.
    void DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode = false);

    // Helper to calculate the absolute screen-space rect for a UI entity, respecting hierarchy.
    Rectangle GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportOffset);

    static UIRenderer& Get();

    inline UIRendererData& GetData()
    {
        return *m_Data;
    }
    inline const UIRendererData& GetData() const
    {
        return *m_Data;
    }

public:
    // Helper scope for UI styling
    struct UIStyleScope
    {
        int ColorPushCount = 0;
        int VarPushCount = 0;
        int FontPushCount = 0;
        float OldFontScale = 1.0f;
        bool Disabled = false;

        UIStyleScope()
        {
        }
        ~UIStyleScope();

        void PushStyle(const UIStyle& style, bool interactable = true);
        void PushText(const TextStyle& text);
        void PushFont(const std::string& fontName, float fontSize = 0.0f);
    };

private:
    // DrawCanvas decomposition helpers
    std::vector<entt::entity> SortUIEntities(entt::registry& registry);
    Rectangle CalculateEntityRect(Entity entity, const Rectangle& parentRect,
                                  std::map<entt::entity, Rectangle>& rectCache);
    bool RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode);

    // Widget specific rendering helpers
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
    void RenderTabBar(const TabBarControl& tb);
    void RenderTabItem(TabItemControl& ti);


private:
    static UIRenderer* s_Instance;
    std::unique_ptr<UIRendererData> m_Data;
};
} // namespace CHEngine

#endif // CH_UI_RENDERER_H

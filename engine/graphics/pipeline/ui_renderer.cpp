#include "ui_renderer.h"
#include "renderer.h"
#include "engine/core/profiler.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <algorithm>
#include <map>

#include "imgui.h"
#include <vector>
#include "entt/entt.hpp"

namespace CHEngine
{
// Internal implementation detail
UIRenderer* UIRenderer::s_Instance = nullptr;

UIRenderer& UIRenderer::Get()
{
    CH_CORE_ASSERT(s_Instance, "UIRenderer not initialized!");
    return *s_Instance;
}

void UIRenderer::Init()
{
    if (!s_Instance) s_Instance = new UIRenderer();
    CH_CORE_INFO("Initializing UIRenderer (ImGui Backend)...");
}

void UIRenderer::Shutdown()
{
    if (s_Instance)
    {
        CH_CORE_INFO("Shutting down UIRenderer...");
        delete s_Instance;
        s_Instance = nullptr;
    }
}

UIRenderer::UIRenderer()
{
}

UIRenderer::~UIRenderer()
{
}

void UIRenderer::LoadProjectFonts()
{
    m_FontRegistry.Clear();
    m_FontRegistry.LoadProjectFonts();
}

// ---------------------------------------------------------------------------
// StyleCounts — replaces UIStyleScope RAII.
// Tracks how many ImGui pushes were made so PopUIStyle() can undo exactly that.
// ---------------------------------------------------------------------------
UIRenderer::StyleCounts UIRenderer::PushUIStyle(const UIStyle& style, bool interactable)
{
    StyleCounts c;

    ImGui::PushStyleColor(ImGuiCol_Button,         ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ToImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ToImVec4(style.PressedColor));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,        ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_Border,         ToImVec4(style.BorderColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ToImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ToImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ToImVec4(style.PressedColor));
    c.colors += 8;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,  style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.BorderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,   ImVec2(style.Padding, style.Padding));
    c.vars += 4;

    if (!interactable)
    {
        ImGui::BeginDisabled(true);
        c.disabled = true;
    }

    return c;
}

void UIRenderer::PushTextStyle(const TextStyle& text, StyleCounts& c)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ToImVec4(text.TextColor));
    c.colors++;

    float hAlign = (text.HorizontalAlignment == TextAlignment::Left)   ? 0.0f
                 : (text.HorizontalAlignment == TextAlignment::Center)  ? 0.5f : 1.0f;
    float vAlign = (text.VerticalAlignment == TextAlignment::Top)       ? 0.0f
                 : (text.VerticalAlignment == TextAlignment::Center)    ? 0.5f : 1.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(hAlign, vAlign));
    c.vars++;

    // Font: resolve via UIFontRegistry
    const std::string& fontName = text.FontName;
    if (!fontName.empty() && fontName != "Default")
    {
        ImFont* font = UIRenderer::Get().GetFontRegistry().GetFont(fontName, text.FontSize);
        if (font)
        {
            ImGui::PushFont(font);
            c.fonts++;
        }
    }
}

void UIRenderer::PopUIStyle(const StyleCounts& c)
{
    for (int i = 0; i < c.fonts; ++i)
        ImGui::PopFont();
    ImGui::PopStyleVar(c.vars);
    ImGui::PopStyleColor(c.colors);
    if (c.disabled)
        ImGui::EndDisabled();
}

// --- Modular Rendering Helpers ---------------------------------------------

void UIRenderer::RenderPanel(const PanelControl& panel, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* drawList  = ImGui::GetWindowDrawList();
    ImU32 bgColor         = ImGui::GetColorU32(ToImVec4(panel.Style.BackgroundColor));
    ImU32 borderColor     = ImGui::GetColorU32(ToImVec4(panel.Style.BorderColor));

    if (panel.Texture && panel.Texture->IsReady())
    {
        ImTextureID texId = (ImTextureID)(uintptr_t)panel.Texture->GetTexture().id;
        drawList->AddImageRounded(texId, pos,
                                  {pos.x + size.x, pos.y + size.y}, {0, 0}, {1, 1}, IM_COL32_WHITE,
                                  panel.Style.Rounding);
    }
    else
    {
        drawList->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, bgColor, panel.Style.Rounding);
    }

    if (panel.Style.BorderSize > 0.0f)
    {
        drawList->AddRect(pos, {pos.x + size.x, pos.y + size.y}, borderColor, panel.Style.Rounding, 0,
                          panel.Style.BorderSize);
    }
}

void UIRenderer::RenderLabel(const LabelControl& label, const ImVec2& size)
{
    StyleCounts c;
    PushTextStyle(label.Style, c);

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
    ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str(), nullptr, true, size.x);

    float startX = 0, startY = 0;
    if (label.Style.HorizontalAlignment == TextAlignment::Center)
        startX = (size.x - textSize.x) * 0.5f;
    else if (label.Style.HorizontalAlignment == TextAlignment::Right)
        startX = (size.x - textSize.x);

    if (label.Style.VerticalAlignment == TextAlignment::Center)
        startY = (size.y - textSize.y) * 0.5f;
    else if (label.Style.VerticalAlignment == TextAlignment::Bottom)
        startY = (size.y - textSize.y);

    ImGui::SetCursorPos({ImGui::GetCursorPosX() + startX, ImGui::GetCursorPosY() + startY});
    ImGui::TextUnformatted(label.Text.c_str());
    ImGui::PopTextWrapPos();

    PopUIStyle(c);
}

void UIRenderer::RenderButton(Entity entity, ButtonControl& button, const ImVec2& size, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(button.Style, button.IsInteractable);
    PushTextStyle(button.Text, c);

    if (ImGui::Button(button.Label.c_str(), size))
    {
        CH_CORE_INFO("UIRenderer: Button '{}' (EntityID: {}) clicked in ImGui", button.Label, (int)entity);
        button.PressedThisFrame = true;
    }
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderSlider(SliderControl& slider, const ImVec2& size, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(slider.Style);
    PushTextStyle(slider.Text, c);

    ImGui::SetNextItemWidth(size.x);
    slider.Changed = ImGui::SliderFloat(slider.Label.c_str(), &slider.Value, slider.Min, slider.Max);
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderCheckbox(CheckboxControl& cb, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(cb.Style);
    PushTextStyle(cb.Text, c);

    cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderImage(const ImageControl& image, const ImVec2& size)
{
    auto& assetManager = AssetManager::Get();
    if (!image.TexturePath.empty())
    {
        auto texAsset = assetManager.Get<TextureAsset>(image.TexturePath);
        if (texAsset)
        {
            ImTextureID tid = (ImTextureID)(uintptr_t)texAsset->GetTexture().id;
            ImVec4 tint   = ToImVec4(image.TintColor);
            ImVec4 border = ToImVec4(image.BorderColor);
            ImGui::Image(tid, size, {0, 0}, {1, 1}, tint, border);
        }
    }
}

void UIRenderer::RenderInputText(Entity entity, InputTextControl& it, const ImVec2& size, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(it.BoxStyle, !it.ReadOnly);
    PushTextStyle(it.Style, c);

    auto& buffer = it.InputBuffer;
    if (buffer.size() != (size_t)it.MaxLength + 1)
    {
        buffer.resize(it.MaxLength + 1, '\0');
        strncpy(buffer.data(), it.Text.c_str(), it.MaxLength);
    }

    ImGuiInputTextFlags flags =
        (it.ReadOnly ? ImGuiInputTextFlags_ReadOnly : 0) | (it.Password ? ImGuiInputTextFlags_Password : 0);
    if (it.Multiline)
    {
        if (ImGui::InputTextMultiline(it.Label.c_str(), buffer.data(), buffer.size(), size, flags))
        {
            it.Text    = buffer.data();
            it.Changed = true;
        }
    }
    else
    {
        ImGui::SetNextItemWidth(size.x);
        if (ImGui::InputText(it.Label.c_str(), buffer.data(), buffer.size(), flags))
        {
            it.Text    = buffer.data();
            it.Changed = true;
        }
    }
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderProgressBar(const ProgressBarControl& pb, const ImVec2& size)
{
    StyleCounts c = PushUIStyle(pb.BarStyle);
    PushTextStyle(pb.Style, c);

    std::string overlay = pb.OverlayText;
    if (overlay.empty() && pb.ShowPercentage)
        overlay = std::to_string((int)(pb.Progress * 100)) + "%";

    ImGui::ProgressBar(pb.Progress, size, overlay.c_str());
    PopUIStyle(c);
}

void UIRenderer::RenderComboBox(ComboBoxControl& cb, const ImVec2& size, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(cb.BoxStyle);
    PushTextStyle(cb.Style, c);

    ImGui::SetNextItemWidth(size.x);
    const char* preview = (cb.SelectedIndex >= 0 && cb.SelectedIndex < (int)cb.Items.size())
                              ? cb.Items[cb.SelectedIndex].c_str()
                              : "";
    if (ImGui::BeginCombo(cb.Label.c_str(), preview))
    {
        for (int i = 0; i < (int)cb.Items.size(); i++)
        {
            if (ImGui::Selectable(cb.Items[i].c_str(), i == cb.SelectedIndex))
            {
                cb.SelectedIndex = i;
                cb.Changed       = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderImageButton(ImageButtonControl& ib, const ImVec2& size, bool& itemHandled)
{
    auto& assetManager = AssetManager::Get();
    if (!ib.TexturePath.empty())
    {
        auto tex = assetManager.Get<TextureAsset>(ib.TexturePath);
        if (tex)
        {
            ImTextureID tid = (ImTextureID)(uintptr_t)tex->GetTexture().id;
            ImVec4 bg   = ToImVec4(ib.BackgroundColor);
            ImVec4 tint = ToImVec4(ib.TintColor);
            if (ImGui::ImageButton(ib.Label.c_str(), tid, size, {0, 0}, {1, 1}, bg, tint))
                ib.PressedThisFrame = true;
            if (ImGui::IsItemActive())
                itemHandled = true;
        }
    }
}

void UIRenderer::RenderRadioButton(RadioButtonControl& rb, bool& itemHandled)
{
    StyleCounts c;
    PushTextStyle(rb.Style, c);

    for (int i = 0; i < (int)rb.Options.size(); i++)
    {
        if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i))
        {
            rb.SelectedIndex = i;
            rb.Changed       = true;
        }
        if (rb.Horizontal && i < (int)rb.Options.size() - 1)
            ImGui::SameLine();
    }
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderColorPicker(ColorPickerControl& cp, bool& itemHandled)
{
    float col[4] = {cp.SelectedColor.r / 255.f, cp.SelectedColor.g / 255.f,
                    cp.SelectedColor.b / 255.f, cp.SelectedColor.a / 255.f};
    if (cp.ShowPicker ? ImGui::ColorPicker4(cp.Label.c_str(), col)
                      : ImGui::ColorEdit4(cp.Label.c_str(), col))
    {
        cp.SelectedColor = {(uint8_t)(col[0] * 255), (uint8_t)(col[1] * 255),
                            (uint8_t)(col[2] * 255), (uint8_t)(col[3] * 255)};
        cp.Changed = true;
    }
    if (ImGui::IsItemActive())
        itemHandled = true;
}

void UIRenderer::RenderSeparator(const SeparatorControl& sep)
{
    ImGui::PushStyleColor(ImGuiCol_Separator, ToImVec4(sep.LineColor));
    ImGui::Separator();
    ImGui::PopStyleColor();
}

void UIRenderer::RenderDragFloat(DragFloatControl& df, const ImVec2& size, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(df.BoxStyle);
    PushTextStyle(df.Style, c);

    ImGui::SetNextItemWidth(size.x);
    df.Changed = ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max, df.Format.c_str());
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderDragInt(DragIntControl& di, const ImVec2& size, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(di.BoxStyle);
    PushTextStyle(di.Style, c);

    ImGui::SetNextItemWidth(size.x);
    di.Changed = ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max, di.Format.c_str());
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderTreeNode(TreeNodeControl& tn, bool& itemHandled)
{
    StyleCounts c;
    PushTextStyle(tn.Style, c);

    ImGuiTreeNodeFlags flags = (tn.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0) |
                               (tn.IsLeaf ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
    tn.IsOpen = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderCollapsingHeader(CollapsingHeaderControl& ch, bool& itemHandled)
{
    StyleCounts c;
    PushTextStyle(ch.Style, c);

    ch.IsOpen = ImGui::CollapsingHeader(ch.Label.c_str(), ch.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderPlotLines(const PlotLinesControl& pl, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(pl.BoxStyle);
    PushTextStyle(pl.Style, c);

    ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(),
                     pl.ScaleMin, pl.ScaleMax, {pl.GraphSize.x, pl.GraphSize.y});
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

void UIRenderer::RenderPlotHistogram(const PlotHistogramControl& ph, bool& itemHandled)
{
    StyleCounts c = PushUIStyle(ph.BoxStyle);
    PushTextStyle(ph.Style, c);

    ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(),
                         ph.ScaleMin, ph.ScaleMax, {ph.GraphSize.x, ph.GraphSize.y});
    if (ImGui::IsItemActive())
        itemHandled = true;

    PopUIStyle(c);
}

// TabBar iterates over children from the registry.
// Children are identified via HierarchyComponent and sorted by ZOrder.
void UIRenderer::RenderTabBar(Entity tabBarEntity, const TabBarControl& tb, entt::registry& registry)
{
    ImGuiTabBarFlags flags = (tb.Reorderable ? ImGuiTabBarFlags_Reorderable : 0) |
                             (tb.AutoSelectNewTabs ? ImGuiTabBarFlags_AutoSelectNewTabs : 0);
    if (!ImGui::BeginTabBar(tb.Label.c_str(), flags))
        return;

    // Collect direct children that have TabItemControl, sorted by ZOrder.
    std::vector<entt::entity> tabItems;
    if (tabBarEntity.HasComponent<HierarchyComponent>())
    {
        auto& hierarchy = tabBarEntity.GetComponent<HierarchyComponent>();
        for (auto childID : hierarchy.Children)
        {
            if (registry.valid(childID) && registry.all_of<TabItemControl, ControlComponent>(childID))
                tabItems.push_back(childID);
        }
        std::sort(tabItems.begin(), tabItems.end(), [&](entt::entity a, entt::entity b) {
            return registry.get<ControlComponent>(a).ZOrder < registry.get<ControlComponent>(b).ZOrder;
        });
    }

    for (auto childID : tabItems)
    {
        auto& ti = registry.get<TabItemControl>(childID);
        if (ImGui::BeginTabItem(ti.Label.c_str(), &ti.IsOpen))
        {
            ti.Selected = true;
            ImGui::EndTabItem();
        }
        else
        {
            ti.Selected = false;
        }
    }

    ImGui::EndTabBar();
}

// ---------------------------------------------------------------------------
// Sorting / Layout helpers
// ---------------------------------------------------------------------------

std::vector<entt::entity> UIRenderer::SortUIEntities(entt::registry& registry)
{
    auto uiView = registry.view<ControlComponent>();
    std::vector<entt::entity> sorted;
    for (auto entityID : uiView)
        sorted.push_back(entityID);

    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return uiView.get<ControlComponent>(a).ZOrder < uiView.get<ControlComponent>(b).ZOrder;
    });
    return sorted;
}

UIRect UIRenderer::CalculateEntityRect(Entity entity, const UIRect& canvasRect,
                           std::map<entt::entity, UIRect>& rectCache)
{
    auto& control = entity.GetComponent<ControlComponent>();

    // 1. Resolve Parent Rect
    UIRect parentRect = canvasRect;
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
        if (parentID != entt::null)
        {
            if (rectCache.count(parentID))
            {
                parentRect = rectCache[parentID];
            }
            else
            {
                Entity parentEntity{parentID, entity.GetRegistry()};
                if (parentEntity.HasComponent<ControlComponent>())
                {
                    parentRect = CalculateEntityRect(parentEntity, canvasRect, rectCache);
                    rectCache[parentID] = parentRect;
                }
            }
        }
    }

    // 2. Handle AutoSize
    if (entity.HasComponent<LabelControl>() && entity.GetComponent<LabelControl>().AutoSize)
    {
        auto& label     = entity.GetComponent<LabelControl>();
        ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str());
        control.Transform.OffsetMax = {control.Transform.OffsetMin.x + textSize.x + 10.0f,
                                       control.Transform.OffsetMin.y + textSize.y + 4.0f};
    }
    else if (entity.HasComponent<ButtonControl>() && entity.GetComponent<ButtonControl>().AutoSize)
    {
        auto& button    = entity.GetComponent<ButtonControl>();
        ImVec2 textSize = ImGui::CalcTextSize(button.Label.c_str());
        float pad = button.Style.Padding * 2.0f;
        control.Transform.OffsetMax = {control.Transform.OffsetMin.x + textSize.x + pad + 10.0f,
                                       control.Transform.OffsetMin.y + textSize.y + pad + 4.0f};
    }

    // 3. Final Calculation
    Rectangle r = control.Transform.CalculateRect({parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
    return {r.x, r.y, r.width, r.height};
}

UIRect UIRenderer::GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportPos)
{
    if (!entity || !entity.HasComponent<ControlComponent>())
        return { 0, 0, 0, 0 };
    std::map<entt::entity, UIRect> emptyCache;
    UIRect canvas = { viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y };
    return CalculateEntityRect(entity, canvas, emptyCache);
}

// ---------------------------------------------------------------------------

void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
{
    CH_CORE_ASSERT(scene, "Scene is null!");
    CH_PROFILE_FUNCTION();

    ImVec2 currentRefSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
    auto& registry = scene->GetRegistry();

    // --- Canvas Scaling ---
    const CanvasSettings& canvas = scene->GetSettings().Canvas;
    float scaleFactor = 1.0f;
    if (canvas.ScaleMode == CanvasScaleMode::ScaleWithScreenSize &&
        canvas.ReferenceResolution.x > 0 && canvas.ReferenceResolution.y > 0)
    {
        float scaleX = currentRefSize.x / canvas.ReferenceResolution.x;
        float scaleY = currentRefSize.y / canvas.ReferenceResolution.y;
        float t      = canvas.MatchWidthOrHeight;
        scaleFactor  = scaleX * (1.0f - t) + scaleY * t;
    }

    float virtualW  = currentRefSize.x / scaleFactor;
    float virtualH  = currentRefSize.y / scaleFactor;
    float virtualOX = referencePosition.x / scaleFactor;
    float virtualOY = referencePosition.y / scaleFactor;

    std::vector<entt::entity> sortedEntities = SortUIEntities(registry);
    std::map<entt::entity, UIRect> rectCache;

    UIRect canvasRect = {virtualOX, virtualOY, virtualW, virtualH};

    for (entt::entity id : sortedEntities)
    {
        Entity entity(id, &registry);
        auto& control = registry.get<ControlComponent>(id);
        if (!control.IsActive)
            continue;

        // Reset frame flags
        if (entity.HasComponent<ButtonControl>())
            entity.GetComponent<ButtonControl>().PressedThisFrame = false;

        UIRect rect = CalculateEntityRect(entity, canvasRect, rectCache);
        rectCache[id] = rect;

        // Scale virtual rect back to actual screen pixels
        ImVec2 screenPos = {rect.x * scaleFactor, rect.y * scaleFactor};
        ImVec2 size      = {rect.width  * scaleFactor, rect.height * scaleFactor};

        ImGui::SetCursorScreenPos(screenPos);
        ImGui::BeginGroup();
        ImGui::PushID((int)id);

        bool itemHandled = RenderUIComponent(entity, screenPos, size, editMode);

        // --- Edit Mode Dragging ---
        if (editMode)
        {
            if (!itemHandled)
            {
                ImGui::SetCursorScreenPos(screenPos);
                ImGui::InvisibleButton("##DragZone", size);
            }

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                control.Transform.OffsetMin.x += delta.x / scaleFactor;
                control.Transform.OffsetMax.x += delta.x / scaleFactor;
                control.Transform.OffsetMin.y += delta.y / scaleFactor;
                control.Transform.OffsetMax.y += delta.y / scaleFactor;
            }
        }

        ImGui::PopID();
        ImGui::EndGroup();
    }
}

bool UIRenderer::RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode)
{
    bool itemHandled = false;
    auto& registry = entity.GetRegistry();

    if (entity.HasComponent<PanelControl>())
        RenderPanel(entity.GetComponent<PanelControl>(), screenPos, size);

    if (entity.HasComponent<LabelControl>())
        RenderLabel(entity.GetComponent<LabelControl>(), size);

    if (entity.HasComponent<ButtonControl>())
        RenderButton(entity, entity.GetComponent<ButtonControl>(), size, itemHandled);

    if (entity.HasComponent<SliderControl>())
        RenderSlider(entity.GetComponent<SliderControl>(), size, itemHandled);

    if (entity.HasComponent<CheckboxControl>())
        RenderCheckbox(entity.GetComponent<CheckboxControl>(), itemHandled);

    if (entity.HasComponent<ImageControl>())
        RenderImage(entity.GetComponent<ImageControl>(), size);

    if (entity.HasComponent<InputTextControl>())
        RenderInputText(entity, entity.GetComponent<InputTextControl>(), size, itemHandled);

    if (entity.HasComponent<ProgressBarControl>())
        RenderProgressBar(entity.GetComponent<ProgressBarControl>(), size);

    if (entity.HasComponent<ComboBoxControl>())
        RenderComboBox(entity.GetComponent<ComboBoxControl>(), size, itemHandled);

    if (entity.HasComponent<ImageButtonControl>())
        RenderImageButton(entity.GetComponent<ImageButtonControl>(), size, itemHandled);

    if (entity.HasComponent<RadioButtonControl>())
        RenderRadioButton(entity.GetComponent<RadioButtonControl>(), itemHandled);

    if (entity.HasComponent<ColorPickerControl>())
        RenderColorPicker(entity.GetComponent<ColorPickerControl>(), itemHandled);

    if (entity.HasComponent<SeparatorControl>())
        RenderSeparator(entity.GetComponent<SeparatorControl>());

    if (entity.HasComponent<DragFloatControl>())
        RenderDragFloat(entity.GetComponent<DragFloatControl>(), size, itemHandled);

    if (entity.HasComponent<DragIntControl>())
        RenderDragInt(entity.GetComponent<DragIntControl>(), size, itemHandled);

    if (entity.HasComponent<TreeNodeControl>())
        RenderTreeNode(entity.GetComponent<TreeNodeControl>(), itemHandled);

    if (entity.HasComponent<CollapsingHeaderControl>())
        RenderCollapsingHeader(entity.GetComponent<CollapsingHeaderControl>(), itemHandled);

    if (entity.HasComponent<PlotLinesControl>())
        RenderPlotLines(entity.GetComponent<PlotLinesControl>(), itemHandled);

    if (entity.HasComponent<PlotHistogramControl>())
        RenderPlotHistogram(entity.GetComponent<PlotHistogramControl>(), itemHandled);

    if (entity.HasComponent<TabBarControl>())
    {
        RenderTabBar(entity, entity.GetComponent<TabBarControl>(), registry);
        itemHandled = true; // TabBar consumes interaction
    }

    // TabItem is rendered by its parent TabBar — skip standalone rendering.

    return itemHandled;
}

} // namespace CHEngine

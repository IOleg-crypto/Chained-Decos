#include "ui_renderer.h"
#include "renderer.h"
#include "engine/core/profiler.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/font_asset.h"
#include "engine/graphics/texture_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <algorithm>
#include <map>

namespace CHEngine
{
UIRenderer& UIRenderer::Get()
{
    return Renderer::Get().GetUIRenderer();
}

void UIRenderer::Init()
{
    CH_CORE_INFO("Initializing UIRenderer (ImGui Backend)...");
}

void UIRenderer::Shutdown()
{
    CH_CORE_INFO("Shutting down UIRenderer...");
}

UIRenderer::UIRenderer()
{
    m_Data = std::make_unique<UIRendererData>();
}

UIRenderer::~UIRenderer()
{
}


Rectangle UIRenderer::GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportOffset)
{
    if (!entity || !entity.HasComponent<ControlComponent>())
    {
        return {0, 0, 0, 0};
    }

    auto& control = entity.GetComponent<ControlComponent>();
    Rectangle parentRect = {viewportOffset.x, viewportOffset.y, viewportSize.x, viewportSize.y};

    if (entity.HasComponent<HierarchyComponent>())
    {
        auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
        if (parentID != entt::null)
        {
            Entity parent{parentID, &entity.GetRegistry()};
            if (parent.HasComponent<ControlComponent>())
            {
                parentRect = GetEntityRect(parent, viewportSize, viewportOffset);
            }
        }
    }

    return control.Transform.CalculateRect({parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
}

UIRenderer::UIStyleScope::~UIStyleScope()
{
    if (Disabled)
    {
        ImGui::EndDisabled();
    }
    while (FontPushCount > 0)
    {
        ImGui::PopFont();
        FontPushCount--;
    }
    if (OldFontScale != 1.0f)
    {
        ImGui::SetWindowFontScale(OldFontScale);
    }
    ImGui::PopStyleVar(VarPushCount);
    ImGui::PopStyleColor(ColorPushCount);
}

void UIRenderer::UIStyleScope::PushStyle(const UIStyle& style, bool interactable)
{
    // Inline helpers (imgui_converter.h was removed — no external dependency needed)
    auto toImVec4 = [](Color c) -> ImVec4 { return {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f}; };
    auto toImTex  = [](unsigned int id) -> ImTextureID { return (ImTextureID)(uintptr_t)id; };

    ImGui::PushStyleColor(ImGuiCol_Button,        toImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, toImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  toImVec4(style.PressedColor));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,       toImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_Border,        toImVec4(style.BorderColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       toImVec4(style.BackgroundColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,toImVec4(style.HoverColor));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, toImVec4(style.PressedColor));
    ColorPushCount += 8;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style.Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.BorderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.Padding, style.Padding));
    VarPushCount += 4;

    if (!interactable)
    {
        ImGui::BeginDisabled(true);
        Disabled = true;
    }
}

void UIRenderer::UIStyleScope::PushText(const TextStyle& text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{text.TextColor.r/255.f, text.TextColor.g/255.f, text.TextColor.b/255.f, text.TextColor.a/255.f});
    ColorPushCount++;

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign,
                        ImVec2{text.HorizontalAlignment == TextAlignment::Left
                                   ? 0.0f
                                   : (text.HorizontalAlignment == TextAlignment::Center ? 0.5f : 1.0f),
                               text.VerticalAlignment == TextAlignment::Top
                                   ? 0.0f
                                   : (text.VerticalAlignment == TextAlignment::Center ? 0.5f : 1.0f)});
    VarPushCount++;

    if (!text.FontName.empty() && text.FontName != "Default")
    {
        PushFont(text.FontName, text.FontSize);
    }
    else if (text.FontSize > 0.0f)
    {
        PushFont("", text.FontSize);
    }
}

void UIRenderer::UIStyleScope::PushFont(const std::string& fontName, float fontSize)
{
    if (fontSize > 0.0f)
    {
        OldFontScale = ImGui::GetIO().FontGlobalScale; // Or current window scale
        // If we don't have multiple fonts loaded in the atlas yet,
        // the best we can do is scale the current window font.
        // Default size in editor is usually 18.0f or 20.0f (BaseFontSize)
        float baseSize = 20.0f; // This should ideally come from EditorLayer or EditorGUI
        ImGui::SetWindowFontScale(fontSize / baseSize);
    }

    if (fontName.empty() || fontName == "Default")
    {
        return;
    }

    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& am = AssetManager::Get();
    auto fontAsset = am.Get<FontAsset>(fontName);

    // Note: For real ImGui font switching, we need to ensure the font is loaded into ImGui atlas.
    // For now we rely on SetWindowFontScale as requested.
}

// --- Modular Rendering Helpers ---

void UIRenderer::RenderPanel(const PanelControl& panel, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_ChildBg);
    ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);

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
    UIStyleScope scope;
    scope.PushText(label.Style);

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
    ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str(), nullptr, true, size.x);

    float startX = 0, startY = 0;
    if (label.Style.HorizontalAlignment == TextAlignment::Center)
    {
        startX = (size.x - textSize.x) * 0.5f;
    }
    else if (label.Style.HorizontalAlignment == TextAlignment::Right)
    {
        startX = (size.x - textSize.x);
    }

    if (label.Style.VerticalAlignment == TextAlignment::Center)
    {
        startY = (size.y - textSize.y) * 0.5f;
    }
    else if (label.Style.VerticalAlignment == TextAlignment::Bottom)
    {
        startY = (size.y - textSize.y);
    }

    ImGui::SetCursorPos({ImGui::GetCursorPosX() + startX, ImGui::GetCursorPosY() + startY});
    ImGui::TextUnformatted(label.Text.c_str());
    ImGui::PopTextWrapPos();
}

void UIRenderer::RenderButton(Entity entity, ButtonControl& button, const ImVec2& size, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(button.Style, button.IsInteractable);
    scope.PushText(button.Text);
    if (ImGui::Button(button.Label.c_str(), size))
    {
        CH_CORE_INFO("UIRenderer: Button '{}' (EntityID: {}) clicked in ImGui", button.Label, (int)entity);
        button.PressedThisFrame = true;
    }
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderSlider(SliderControl& slider, const ImVec2& size, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(slider.Style);
    scope.PushText(slider.Text);
    ImGui::SetNextItemWidth(size.x);
    slider.Changed = ImGui::SliderFloat(slider.Label.c_str(), &slider.Value, slider.Min, slider.Max);
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderCheckbox(CheckboxControl& cb, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(cb.Style);
    scope.PushText(cb.Text);
    cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
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
            ImVec4 tint   = {image.TintColor.r/255.f,   image.TintColor.g/255.f,   image.TintColor.b/255.f,   image.TintColor.a/255.f};
            ImVec4 border = {image.BorderColor.r/255.f, image.BorderColor.g/255.f, image.BorderColor.b/255.f, image.BorderColor.a/255.f};
            ImGui::Image(tid, size, {0, 0}, {1, 1}, tint, border);
        }
    }
}

void UIRenderer::RenderInputText(Entity entity, InputTextControl& it, const ImVec2& size, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(it.BoxStyle, !it.ReadOnly);
    scope.PushText(it.Style);

    entt::entity id = (entt::entity)entity;
    auto& buffer = m_Data->InputBuffers[id];
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
            it.Text = buffer.data();
            it.Changed = true;
        }
    }
    else
    {
        ImGui::SetNextItemWidth(size.x);
        if (ImGui::InputText(it.Label.c_str(), buffer.data(), buffer.size(), flags))
        {
            it.Text = buffer.data();
            it.Changed = true;
        }
    }
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderProgressBar(const ProgressBarControl& pb, const ImVec2& size)
{
    UIStyleScope scope;
    scope.PushStyle(pb.BarStyle);
    scope.PushText(pb.Style);
    std::string overlay = pb.OverlayText;
    if (overlay.empty() && pb.ShowPercentage)
    {
        overlay = std::to_string((int)(pb.Progress * 100)) + "%";
    }
    ImGui::ProgressBar(pb.Progress, size, overlay.c_str());
}

void UIRenderer::RenderComboBox(ComboBoxControl& cb, const ImVec2& size, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(cb.BoxStyle);
    scope.PushText(cb.Style);
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
                cb.Changed = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
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
            ImVec4 bg   = {ib.BackgroundColor.r/255.f, ib.BackgroundColor.g/255.f, ib.BackgroundColor.b/255.f, ib.BackgroundColor.a/255.f};
            ImVec4 tint = {ib.TintColor.r/255.f,       ib.TintColor.g/255.f,       ib.TintColor.b/255.f,       ib.TintColor.a/255.f};
            if (ImGui::ImageButton(ib.Label.c_str(), tid, size, {0, 0}, {1, 1}, bg, tint))
            {
                ib.PressedThisFrame = true;
            }
            if (ImGui::IsItemActive())
            {
                itemHandled = true;
            }
        }
    }
}

void UIRenderer::RenderRadioButton(RadioButtonControl& rb, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushText(rb.Style);
    for (int i = 0; i < (int)rb.Options.size(); i++)
    {
        if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i))
        {
            rb.SelectedIndex = i;
            rb.Changed = true;
        }
        if (rb.Horizontal && i < (int)rb.Options.size() - 1)
        {
            ImGui::SameLine();
        }
    }
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderColorPicker(ColorPickerControl& cp, bool& itemHandled)
{
    float c[4] = {cp.SelectedColor.r / 255.f, cp.SelectedColor.g / 255.f, cp.SelectedColor.b / 255.f,
                  cp.SelectedColor.a / 255.f};
    if (cp.ShowPicker ? ImGui::ColorPicker4(cp.Label.c_str(), c) : ImGui::ColorEdit4(cp.Label.c_str(), c))
    {
        cp.SelectedColor = {(uint8_t)(c[0] * 255), (uint8_t)(c[1] * 255), (uint8_t)(c[2] * 255),
                            (uint8_t)(c[3] * 255)};
        cp.Changed = true;
    }
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderSeparator(const SeparatorControl& sep)
{
    ImVec4 lineImColor = {sep.LineColor.r/255.f, sep.LineColor.g/255.f, sep.LineColor.b/255.f, sep.LineColor.a/255.f};
    ImGui::PushStyleColor(ImGuiCol_Separator, lineImColor);
    ImGui::Separator();
    ImGui::PopStyleColor();
}

void UIRenderer::RenderDragFloat(DragFloatControl& df, const ImVec2& size, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(df.BoxStyle);
    scope.PushText(df.Style);
    ImGui::SetNextItemWidth(size.x);
    df.Changed = ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max, df.Format.c_str());
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderDragInt(DragIntControl& di, const ImVec2& size, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(di.BoxStyle);
    scope.PushText(di.Style);
    ImGui::SetNextItemWidth(size.x);
    di.Changed = ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max, di.Format.c_str());
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderTreeNode(TreeNodeControl& tn, bool& itemHandled)
{
    ImGuiTreeNodeFlags flags = (tn.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0) |
                               (tn.IsLeaf ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
    tn.IsOpen = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderCollapsingHeader(CollapsingHeaderControl& ch, bool& itemHandled)
{
    ch.IsOpen = ImGui::CollapsingHeader(ch.Label.c_str(), ch.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderPlotLines(const PlotLinesControl& pl, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(pl.BoxStyle);
    scope.PushText(pl.Style);
    ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(),
                     pl.ScaleMin, pl.ScaleMax, {pl.GraphSize.x, pl.GraphSize.y});
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderPlotHistogram(const PlotHistogramControl& ph, bool& itemHandled)
{
    UIStyleScope scope;
    scope.PushStyle(ph.BoxStyle);
    scope.PushText(ph.Style);
    ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(),
                         ph.ScaleMin, ph.ScaleMax, {ph.GraphSize.x, ph.GraphSize.y});
    if (ImGui::IsItemActive())
    {
        itemHandled = true;
    }
}

void UIRenderer::RenderTabBar(const TabBarControl& tb)
{
    ImGuiTabBarFlags flags = (tb.Reorderable ? ImGuiTabBarFlags_Reorderable : 0) |
                             (tb.AutoSelectNewTabs ? ImGuiTabBarFlags_AutoSelectNewTabs : 0);
    if (ImGui::BeginTabBar(tb.Label.c_str(), flags))
    {
        ImGui::EndTabBar();
    }
}

void UIRenderer::RenderTabItem(TabItemControl& ti)
{
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
        float t = canvas.MatchWidthOrHeight; // 0 = match width, 1 = match height
        scaleFactor = scaleX * (1.0f - t) + scaleY * t;
    }

    // Build a virtual canvas rect in reference-resolution space, scaled to screen
    // All CalculateEntityRect calls will use this rect, so offsets scale automatically
    float virtualW = currentRefSize.x / scaleFactor;
    float virtualH = currentRefSize.y / scaleFactor;
    float virtualOX = referencePosition.x / scaleFactor;
    float virtualOY = referencePosition.y / scaleFactor;

    std::vector<entt::entity> sortedEntities = SortUIEntities(registry);
    std::map<entt::entity, Rectangle> rectCache; // rects stored in virtual (unscaled) space

    Rectangle canvasRect = {virtualOX, virtualOY, virtualW, virtualH};

    for (entt::entity id : sortedEntities)
    {
        Entity entity(id, &registry);
        auto& control = registry.get<ControlComponent>(id);
        if (!control.IsActive)
        {
            continue;
        }

        // Reset frame flags
        if (entity.HasComponent<ButtonControl>())
        {
            entity.GetComponent<ButtonControl>().PressedThisFrame = false;
        }

        Rectangle rect = CalculateEntityRect(entity, canvasRect, rectCache);
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
                // Convert mouse delta back to virtual space for consistent editing
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

std::vector<entt::entity> UIRenderer::SortUIEntities(entt::registry& registry)
{
    auto uiView = registry.view<ControlComponent>();
    std::vector<entt::entity> sorted;
    for (auto entityID : uiView)
    {
        sorted.push_back(entityID);
    }

    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return uiView.get<ControlComponent>(a).ZOrder < uiView.get<ControlComponent>(b).ZOrder;
    });

    return sorted;
}

Rectangle UIRenderer::CalculateEntityRect(Entity entity, const Rectangle& canvasRect,
                                          std::map<entt::entity, Rectangle>& rectCache)
{
    auto& control = entity.GetComponent<ControlComponent>();

    // 1. Resolve Parent Rect
    Rectangle parentRect = canvasRect;
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
        if (parentID != entt::null && rectCache.count(parentID))
        {
            parentRect = rectCache[parentID];
        }
    }

    // 2. Handle AutoSize
    if (entity.HasComponent<LabelControl>() && entity.GetComponent<LabelControl>().AutoSize)
    {
        auto& label = entity.GetComponent<LabelControl>();
        ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str());
        control.Transform.OffsetMax = {control.Transform.OffsetMin.x + textSize.x + 10.0f,
                                       control.Transform.OffsetMin.y + textSize.y + 4.0f};
    }
    else if (entity.HasComponent<ButtonControl>() && entity.GetComponent<ButtonControl>().AutoSize)
    {
        auto& button = entity.GetComponent<ButtonControl>();
        ImVec2 textSize = ImGui::CalcTextSize(button.Label.c_str());
        float pad = button.Style.Padding * 2.0f;
        control.Transform.OffsetMax = {control.Transform.OffsetMin.x + textSize.x + pad + 10.0f,
                                       control.Transform.OffsetMin.y + textSize.y + pad + 4.0f};
    }

    // 3. Final Calculation
    return control.Transform.CalculateRect({parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
}

bool UIRenderer::RenderUIComponent(Entity entity, const ImVec2& screenPos, const ImVec2& size, bool editMode)
{
    bool itemHandled = false;

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
        RenderTabBar(entity.GetComponent<TabBarControl>());

    if (entity.HasComponent<TabItemControl>())
        RenderTabItem(entity.GetComponent<TabItemControl>());

    return itemHandled;
}
} // namespace CHEngine

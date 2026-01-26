#include "canvas_renderer.h"
#include "engine/render/asset_manager.h"
#include "engine/render/font_asset.h"
#include "engine/render/texture_asset.h"
#include "engine/scene/components.h"
#include "engine/scene/components/hierarchy_component.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include "font_manager.h"
#include <algorithm>
#include <imgui_internal.h>

namespace CHEngine
{

void CanvasRenderer::DrawEntity(Entity entity, const ImVec2 &parentPos, const ImVec2 &parentSize,
                                bool editMode)
{
    if (!entity || !entity.IsValid() || !entity.HasComponent<WidgetComponent>())
        return;

    auto &base = entity.GetComponent<WidgetComponent>();
    if (!base.IsActive)
        return;

    ImGui::PushID((int)(uint32_t)entity);

    // 1. Calculate RectTransform
    ImVec2 anchorMin = {parentPos.x + base.Transform.AnchorMin.x * parentSize.x,
                        parentPos.y + base.Transform.AnchorMin.y * parentSize.y};
    ImVec2 anchorMax = {parentPos.x + base.Transform.AnchorMax.x * parentSize.x,
                        parentPos.y + base.Transform.AnchorMax.y * parentSize.y};

    ImVec2 p0 = {anchorMin.x + base.Transform.OffsetMin.x,
                 anchorMin.y + base.Transform.OffsetMin.y};
    ImVec2 p1 = {anchorMax.x + base.Transform.OffsetMax.x,
                 anchorMax.y + base.Transform.OffsetMax.y};

    ImVec2 size = {p1.x - p0.x, p1.y - p0.y};
    ImVec2 pivotOffset = {size.x * base.Transform.Pivot.x, size.y * base.Transform.Pivot.y};

    // Position Bias (Additive shift relative to anchors)
    ImVec2 finalAbsPos = {p0.x - pivotOffset.x + base.Transform.RectCoordinates.x,
                          p0.y - pivotOffset.y + base.Transform.RectCoordinates.y};

    ImGui::SetCursorScreenPos(finalAbsPos);

    // 2. Specialized Handling
    if (entity.HasComponent<PanelWidget>())
        HandlePanel(entity, finalAbsPos, size);

    if (entity.HasComponent<ButtonWidget>())
        HandleButton(entity, finalAbsPos, size);

    if (entity.HasComponent<LabelWidget>())
        HandleLabel(entity, finalAbsPos, size);

    // 3. Edit Mode Dragging
    if (editMode)
    {
        ImGui::SetCursorScreenPos(finalAbsPos);
        ImGui::InvisibleButton("##drag_handle", size);

        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            base.Transform.RectCoordinates.x += delta.x;
            base.Transform.RectCoordinates.y += delta.y;
        }

        // Visual Feedback in Editor
        if (hovered || active)
        {
            ImDrawList *fgList = ImGui::GetForegroundDrawList();
            fgList->AddRect(finalAbsPos, {finalAbsPos.x + size.x, finalAbsPos.y + size.y},
                            active ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 150), 0.0f, 0,
                            2.0f);
        }
    }
    else
    {
        // Register space used so ImGui doesn't think SetCursorPos was wasted
        ImGui::Dummy(size);
    }

    // 4. Recursive Children with Layout Support
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto &hc = entity.GetComponent<HierarchyComponent>();
        ImVec2 currentChildPos = finalAbsPos;

        bool isVertical = entity.HasComponent<VerticalLayoutGroup>();
        float spacing = isVertical ? entity.GetComponent<VerticalLayoutGroup>().Spacing : 0.0f;
        Vector2 padding =
            isVertical ? entity.GetComponent<VerticalLayoutGroup>().Padding : Vector2{0, 0};

        if (isVertical)
            currentChildPos.y += padding.y;

        for (auto childID : hc.Children)
        {
            Entity child{childID, entity.GetScene()};
            if (!child.IsValid() || !child.HasComponent<WidgetComponent>())
                continue;

            DrawEntity(child, isVertical ? currentChildPos : finalAbsPos, size, editMode);

            if (isVertical)
            {
                auto &childBase = child.GetComponent<WidgetComponent>();
                float childHeight =
                    (childBase.Transform.OffsetMax.y - childBase.Transform.OffsetMin.y);
                currentChildPos.y += childHeight + spacing;
            }
        }
    }

    ImGui::PopID();
}

void CanvasRenderer::HandleButton(Entity entity, const ImVec2 &pos, const ImVec2 &size)
{
    auto &btn = entity.GetComponent<ButtonWidget>();
    if (!btn.IsInteractable)
        return;

    ImRect bb(pos, {pos.x + size.x, pos.y + size.y});
    ImGui::ItemAdd(bb, ImGui::GetID("##btn_logic"));

    btn.IsHovered = ImGui::IsItemHovered();
    btn.IsDown = btn.IsHovered && ImGui::IsMouseDown(0);
    btn.PressedThisFrame = btn.IsHovered && ImGui::IsMouseReleased(0);

    // Render Background
    DrawStyledRect(pos, {pos.x + size.x, pos.y + size.y}, btn.Style, btn.IsHovered, btn.IsDown);

    // Render Label
    if (!btn.Label.empty())
    {
        DrawStyledText(btn.Label, pos, size, btn.Text);
    }
}

void CanvasRenderer::HandlePanel(Entity entity, const ImVec2 &pos, const ImVec2 &size)
{
    auto &panel = entity.GetComponent<PanelWidget>();

    // Full screen override
    ImVec2 p_min = pos;
    ImVec2 p_max = {pos.x + size.x, pos.y + size.y};

    if (panel.FullScreen)
    {
        p_min = {0, 0};
        p_max = ImGui::GetIO().DisplaySize;
    }

    if (!panel.TexturePath.empty() && !panel.Texture)
    {
        panel.Texture = AssetManager::Get<TextureAsset>(panel.TexturePath);
    }

    if (panel.Texture && panel.Texture->IsReady())
    {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        drawList->AddImageRounded(
            (ImTextureID)(uintptr_t)panel.Texture->GetTexture().id, p_min, p_max, {0, 0}, {1, 1},
            IM_COL32(255, 255, 255, panel.Style.BackgroundColor.a), panel.Style.Rounding);
    }
    else
    {
        DrawStyledRect(p_min, p_max, panel.Style, false, false);
    }
}

void CanvasRenderer::HandleLabel(Entity entity, const ImVec2 &pos, const ImVec2 &size)
{
    auto &label = entity.GetComponent<LabelWidget>();
    DrawStyledText(label.Text, pos, size, label.Style);
}

void CanvasRenderer::DrawStyledRect(const ImVec2 &p_min, const ImVec2 &p_max, const UIStyle &style,
                                    bool isHovered, bool isPressed)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    Color col = style.BackgroundColor;
    if (isPressed)
        col = style.PressedColor;
    else if (isHovered)
        col = style.HoverColor;

    ImU32 col32 = IM_COL32(col.r, col.g, col.b, col.a);

    if (style.bUseGradient)
    {
        ImU32 colBg2 = IM_COL32(style.GradientColor.r, style.GradientColor.g, style.GradientColor.b,
                                style.GradientColor.a);
        drawList->AddRectFilledMultiColor(p_min, p_max, col32, col32, colBg2, colBg2);
    }
    else
    {
        drawList->AddRectFilled(p_min, p_max, col32, style.Rounding);
    }

    if (style.BorderSize > 0)
    {
        drawList->AddRect(p_min, p_max,
                          IM_COL32(style.BorderColor.r, style.BorderColor.g, style.BorderColor.b,
                                   style.BorderColor.a),
                          style.Rounding, 0, style.BorderSize);
    }
}

void CanvasRenderer::DrawStyledText(const std::string &text, const ImVec2 &absPos,
                                    const ImVec2 &size, const TextStyle &style)
{
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    // 1. Pick the best font from atlas
    ImFont *font = FontManager::GetFont(style.FontPath, style.FontSize);

    // Simple centering for now
    ImGui::PushFont(font);
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImGui::PopFont();

    ImVec2 textPos = {absPos.x + (size.x - textSize.x) * 0.5f,
                      absPos.y + (size.y - textSize.y) * 0.5f};

    // Support for Raylib Fonts
    if (!style.FontPath.empty())
    {
        auto fontAsset = AssetManager::Get<FontAsset>(style.FontPath);
        if (fontAsset && fontAsset->IsReady())
        {
            // Use Raylib Font directly via ImGui Callback
            // Note: This requires the correct GL context and state management.
            // Since rlImGui manages the atlas, we draw this AFTER ImGui's own text if we want.
            // For now, let's use a simpler approach: use Raylib's text rendering if possible.

            // We draw directly into the current render target (which is likely the Viewport
            // texture) if we are called within the viewport scope.

            float spacing = style.LetterSpacing;
            float fontSize = style.FontSize;
            ::DrawTextEx(fontAsset->GetFont(), text.c_str(), {textPos.x, textPos.y}, fontSize,
                         spacing, style.TextColor);
            return;
        }
    }

    if (style.bShadow)
    {
        ImGui::PushFont(font);
        drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 150),
                          text.c_str());
        ImGui::PopFont();
    }

    ImGui::PushFont(font);
    drawList->AddText(
        textPos,
        IM_COL32(style.TextColor.r, style.TextColor.g, style.TextColor.b, style.TextColor.a),
        text.c_str());
    ImGui::PopFont();
}

} // namespace CHEngine

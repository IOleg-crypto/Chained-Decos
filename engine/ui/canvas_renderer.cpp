#include "canvas_renderer.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/font_asset.h"
#include "engine/renderer/texture_asset.h"
#include "engine/scene/components/hierarchy_component.h"
#include "engine/scene/entity.h"
#include <imgui_internal.h>

namespace CHEngine
{
void CanvasRenderer::DrawEntity(Entity entity, const ImVec2 &parentPos, const ImVec2 &parentSize,
                                bool editMode)
{
    if (!entity || !entity.HasComponent<WidgetComponent>())
        return;

    auto &base = entity.GetComponent<WidgetComponent>();
    if (!base.IsActive)
        return;

    ImGui::PushID((int)(uint32_t)entity);

    // 0. Calculate Absolute Position relative to parent container
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

    ImVec2 finalAbsPos = {p0.x - pivotOffset.x, p0.y - pivotOffset.y};

    // Store for children
    ImVec2 selfAbsPos = finalAbsPos;
    ImVec2 selfSize = size;

    ImGui::SetCursorScreenPos(finalAbsPos);

    // Pre-declare button state for visual feedback
    bool isHovered = false;
    bool isClicked = false;

    // Pre-calculate button state if present
    if (entity.HasComponent<ButtonWidget>())
    {
        auto &btn = entity.GetComponent<ButtonWidget>();

        // Create hit test rect early to determine hover state
        ImRect bb(finalAbsPos, ImVec2(finalAbsPos.x + size.x, finalAbsPos.y + size.y));
        ImGui::ItemAdd(bb, ImGui::GetID("##btn"));

        if (btn.Interactable)
        {
            isHovered = ImGui::IsItemHovered();
            isClicked = isHovered && ImGui::IsMouseClicked(0);
        }
    }

    // 1. Draw Image Background if present
    if (entity.HasComponent<ImageWidget>())
    {
        auto &img = entity.GetComponent<ImageWidget>();

        // Apply visual feedback for button states
        Color finalColor = img.BackgroundColor;
        if (entity.HasComponent<ButtonWidget>())
        {
            auto &btn = entity.GetComponent<ButtonWidget>();
            if (!btn.Interactable)
            {
                // Disabled: gray out
                finalColor = Color{128, 128, 128, 255};
            }
            else if (isHovered)
            {
                // Hovered: brighten
                finalColor = Color{(unsigned char)std::min(255, img.BackgroundColor.r + 30),
                                   (unsigned char)std::min(255, img.BackgroundColor.g + 30),
                                   (unsigned char)std::min(255, img.BackgroundColor.b + 30),
                                   img.BackgroundColor.a};
            }
        }

        DrawImage(img, finalAbsPos, size, finalColor);
    }

    // 2. Draw Text if present
    if (entity.HasComponent<TextWidget>())
    {
        auto &txt = entity.GetComponent<TextWidget>();
        DrawText(txt, finalAbsPos, size);
    }

    // 3. Handle button click event
    if (entity.HasComponent<ButtonWidget>() && isClicked)
    {
        auto &btn = entity.GetComponent<ButtonWidget>();
        btn.Pressed = true;
        if (btn.OnPressed)
            btn.OnPressed();
    }

    if (entity.HasComponent<SliderWidget>())
    {
        auto &slider = entity.GetComponent<SliderWidget>();
        if (ImGui::SliderFloat("##slider", &slider.Value, slider.Min, slider.Max))
            slider.Changed = true;
    }

    if (entity.HasComponent<CheckboxWidget>())
    {
        auto &cb = entity.GetComponent<CheckboxWidget>();
        if (ImGui::Checkbox("##cb", &cb.Checked))
            cb.Changed = true;
    }

    // 4. Edit mode dragging
    if (editMode && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        base.Transform.OffsetMin.x += delta.x;
        base.Transform.OffsetMin.y += delta.y;
        base.Transform.OffsetMax.x += delta.x;
        base.Transform.OffsetMax.y += delta.y;
    }

    // 5. Recursive render children
    if (entity.HasComponent<HierarchyComponent>())
    {
        auto &hc = entity.GetComponent<HierarchyComponent>();
        for (auto childID : hc.Children)
        {
            Entity child{childID, entity.GetScene()};
            DrawEntity(child, selfAbsPos, selfSize, editMode);
        }
    }

    ImGui::PopID();
}

void CanvasRenderer::DrawImage(ImageWidget &img, const ImVec2 &absPos, const ImVec2 &size,
                               const Color &overrideColor)
{
    if (!img.BackgroundColor.a && !img.Texture)
        return;

    // Lazy load
    if (!img.Texture && !img.TexturePath.empty())
        img.Texture = Assets::Get<TextureAsset>(img.TexturePath);

    ImVec2 p_min = absPos;
    ImVec2 p_max = {p_min.x + size.x + img.Padding.x * 2, p_min.y + size.y + img.Padding.y * 2};

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    if (img.Texture && img.Texture->IsReady())
    {
        ImVec2 uv0 = {0, 0}, uv1 = {1, 1};
        // Reuse aspect ratio logic...
        drawList->AddImageRounded(
            (ImTextureID)(uintptr_t)img.Texture->GetTexture().id, p_min, p_max, uv0, uv1,
            IM_COL32(overrideColor.r, overrideColor.g, overrideColor.b, overrideColor.a),
            img.Rounding);
    }
    else
    {
        drawList->AddRectFilled(
            p_min, p_max,
            IM_COL32(overrideColor.r, overrideColor.g, overrideColor.b, overrideColor.a),
            img.Rounding);
    }
}

void CanvasRenderer::DrawText(TextWidget &txt, const ImVec2 &absPos, const ImVec2 &size)
{
    // Lazy load font
    if (!txt.Font && !txt.FontPath.empty())
        txt.Font = Assets::Get<FontAsset>(txt.FontPath);

    // Simple text draw for now
    ImGui::SetCursorScreenPos(absPos);
    ImGui::TextColored(ImVec4(txt.Color.r / 255.0f, txt.Color.g / 255.0f, txt.Color.b / 255.0f,
                              txt.Color.a / 255.0f),
                       "%s", txt.Text.c_str());
}

} // namespace CHEngine

// ui_render_basic.cpp
// Renders: Panel, Button, Label, Checkbox, ProgressBar, Separator
#include "ui_render_helpers.h"
#include <algorithm>

namespace Chained
{

bool RenderPanel(PanelData& panel, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    auto textureAsset = ResolveTexture(panel.TextureHandle, panel.TexturePath);
    if (textureAsset && textureAsset->GetState() == AssetState::Ready)
    {
        if (auto tex = textureAsset->GetTexture())
        {
            ImTextureID texId = (ImTextureID)(uintptr_t)tex->GetNativeHandle();
            dl->AddImageRounded(texId, pos, pMax, {0, 0}, {1, 1}, IM_COL32_WHITE, wc.BoxStyle.Rounding);
        }
    }
    else
    {
        dl->AddRectFilled(pos, pMax, ToImU32(wc.BoxStyle.BackgroundColor), wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }
    return false;
}

bool RenderButton(const ButtonData& button, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font,
                  const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    ImU32 btnColor = GetControlColor(wc.BoxStyle, wc);
    dl->AddRectFilled(pos, pMax, btnColor, wc.BoxStyle.Rounding);

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }

    RenderAlignedTextureText(dl, font, textStyle.FontSize, button.Label, pos, size, textStyle);

    return wc.PressedThisFrame;
}

bool RenderLabel(const LabelData& label, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font,
                 const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    RenderAlignedTextureText(dl, font, textStyle.FontSize, label.Text, pos, size, textStyle);
    return false;
}

bool RenderCheckbox(CheckboxData& cb, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font,
                    const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float boxSize = size.y > 0.0f ? size.y : 18.0f;
    ImVec2 boxMax = {pos.x + boxSize, pos.y + boxSize};

    ImU32 boxColor = GetControlColor(wc.BoxStyle, wc);
    dl->AddRectFilled(pos, boxMax, boxColor, wc.BoxStyle.Rounding);
    dl->AddRect(pos, boxMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, 1.0f);

    if (cb.Checked)
    {
        float pad = boxSize * 0.25f;
        dl->AddRectFilled({pos.x + pad, pos.y + pad}, {boxMax.x - pad, boxMax.y - pad}, ToImU32(textStyle.TextColor),
                          wc.BoxStyle.Rounding);
    }

    ImVec2 textPos = {pos.x + boxSize + 8.0f, pos.y};
    ImVec2 textSize = {size.x - boxSize - 8.0f, size.y};
    RenderAlignedTextureText(dl, font, textStyle.FontSize, cb.Label, textPos, textSize, textStyle);

    if (wc.PressedThisFrame)
    {
        cb.Checked = !cb.Checked;
        return true;
    }

    return false;
}

bool RenderProgressBar(const ProgressBarData& pb, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMax = {pos.x + size.x, pos.y + size.y};

    dl->AddRectFilled(pos, pMax, ToImU32(wc.BoxStyle.BackgroundColor), wc.BoxStyle.Rounding);

    float currentProgressWidth = size.x * std::clamp(pb.Progress, 0.0f, 1.0f);
    if (currentProgressWidth > 0.0f)
    {
        ImVec2 progressMax = {pos.x + currentProgressWidth, pos.y + size.y};
        dl->AddRectFilled(pos, progressMax, ToImU32(wc.BoxStyle.HoverColor), wc.BoxStyle.Rounding);
    }

    if (wc.BoxStyle.BorderSize > 0.0f)
    {
        dl->AddRect(pos, pMax, ToImU32(wc.BoxStyle.BorderColor), wc.BoxStyle.Rounding, 0, wc.BoxStyle.BorderSize);
    }

    return false;
}

bool RenderSeparator(const SeparatorData& sep, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float midY = pos.y + size.y * 0.5f;
    dl->AddLine({pos.x, midY}, {pos.x + size.x, midY}, ToImU32(sep.LineColor), sep.Thickness);
    return false;
}

bool RenderRadioButton(RadioButtonData& radio, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size,
                       ImFont* font, const TextStyle& textStyle)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool changed = false;

    if (radio.Horizontal)
    {
        float itemW = (radio.Options.empty()) ? size.x : size.x / static_cast<float>(radio.Options.size());
        for (int i = 0; i < (int)radio.Options.size(); i++)
        {
            ImVec2 itemPos = {pos.x + itemW * i, pos.y};
            float radius = std::min(size.y * 0.35f, 8.0f);
            ImVec2 circleCenter = {itemPos.x + radius + 4.0f, pos.y + size.y * 0.5f};
            ImU32 circleColor =
                (i == radio.SelectedIndex) ? ToImU32(textStyle.TextColor) : ToImU32(wc.BoxStyle.BorderColor);
            dl->AddCircle(circleCenter, radius, circleColor, 12, 1.5f);
            if (i == radio.SelectedIndex)
            {
                dl->AddCircleFilled(circleCenter, radius * 0.5f, ToImU32(textStyle.TextColor));
            }
            ImVec2 labelPos = {circleCenter.x + radius + 4.0f, pos.y};
            ImVec2 labelSize = {itemW - radius * 2.0f - 12.0f, size.y};
            RenderAlignedTextureText(dl, font, textStyle.FontSize, radio.Options[i], labelPos, labelSize, textStyle);

            ImVec2 itemMax = {itemPos.x + itemW, pos.y + size.y};
            ImGui::SetCursorScreenPos(itemPos);
            if (ImGui::InvisibleButton(("##radio" + std::to_string(i)).c_str(), {itemW, size.y}))
            {
                radio.SelectedIndex = i;
                changed = true;
            }
        }
    }
    else
    {
        float itemH = (radio.Options.empty()) ? size.y : size.y / static_cast<float>(radio.Options.size());
        for (int i = 0; i < (int)radio.Options.size(); i++)
        {
            ImVec2 itemPos = {pos.x, pos.y + itemH * i};
            float radius = std::min(itemH * 0.35f, 8.0f);
            ImVec2 circleCenter = {itemPos.x + radius + 4.0f, itemPos.y + itemH * 0.5f};
            ImU32 circleColor =
                (i == radio.SelectedIndex) ? ToImU32(textStyle.TextColor) : ToImU32(wc.BoxStyle.BorderColor);
            dl->AddCircle(circleCenter, radius, circleColor, 12, 1.5f);
            if (i == radio.SelectedIndex)
            {
                dl->AddCircleFilled(circleCenter, radius * 0.5f, ToImU32(textStyle.TextColor));
            }
            ImVec2 labelPos = {circleCenter.x + radius + 4.0f, itemPos.y};
            ImVec2 labelSize = {size.x - radius * 2.0f - 12.0f, itemH};
            RenderAlignedTextureText(dl, font, textStyle.FontSize, radio.Options[i], labelPos, labelSize, textStyle);

            ImGui::SetCursorScreenPos(itemPos);
            if (ImGui::InvisibleButton(("##radio" + std::to_string(i)).c_str(), {size.x, itemH}))
            {
                radio.SelectedIndex = i;
                changed = true;
            }
        }
    }
    return changed;
}

} // namespace Chained

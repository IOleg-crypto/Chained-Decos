// ui_render_input.cpp
// Renders: Slider, ComboBox, InputText, DragFloat, DragInt, ColorPicker
#include "ui_render_helpers.h"

namespace Chained
{

bool RenderSlider(SliderData& slider, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, wc.BoxStyle.Rounding);
    bool changed = ImGui::SliderFloat("##slider", &slider.Value, slider.Min, slider.Max, "%.2f");
    ImGui::PopStyleVar();

    return changed;
}

bool RenderComboBox(ComboBoxData& combo, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size, ImFont* font, const TextStyle& textStyle)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);

    const char* preview = (combo.SelectedIndex >= 0 && combo.SelectedIndex < (int)combo.Items.size())
        ? combo.Items[combo.SelectedIndex].c_str() : "Select...";

    bool changed = false;
    if (ImGui::BeginCombo("##combo", preview))
    {
        for (int i = 0; i < (int)combo.Items.size(); i++)
        {
            bool isSelected = (combo.SelectedIndex == i);
            if (ImGui::Selectable(combo.Items[i].c_str(), isSelected))
            {
                combo.SelectedIndex = i;
                changed = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool RenderInputText(InputTextData& input, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    if (input.InputBuffer.empty())
    {
        input.InputBuffer.resize(static_cast<size_t>(input.MaxLength) + 1, '\0');
        std::copy(input.Text.begin(),
                  input.Text.begin() + std::min(input.Text.size(), static_cast<size_t>(input.MaxLength)),
                  input.InputBuffer.begin());
    }

    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);

    ImGuiInputTextFlags flags = 0;
    if (input.ReadOnly)   flags |= ImGuiInputTextFlags_ReadOnly;
    if (input.Password)   flags |= ImGuiInputTextFlags_Password;

    bool changed = false;
    if (input.Multiline)
    {
        changed = ImGui::InputTextMultiline("##inputtext", input.InputBuffer.data(),
                                            input.InputBuffer.size(), size, flags);
    }
    else
    {
        changed = ImGui::InputText("##inputtext", input.InputBuffer.data(),
                                   input.InputBuffer.size(), flags);
    }

    if (changed)
        input.Text = input.InputBuffer.data();

    return changed;
}

bool RenderDragFloat(DragFloatData& drag, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragFloat("##dragfloat", &drag.Value, drag.Speed, drag.Min, drag.Max, drag.Format.c_str());
}

bool RenderDragInt(DragIntData& drag, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::SetNextItemWidth(size.x);
    return ImGui::DragInt("##dragint", &drag.Value, drag.Speed, drag.Min, drag.Max, drag.Format.c_str());
}

bool RenderColorPicker(ColorPickerData& picker, UIControlComponent& wc, const ImVec2& pos, const ImVec2& size)
{
    ImGui::SetCursorScreenPos(pos);
    float col[4] = {
        picker.SelectedColor.r / 255.0f,
        picker.SelectedColor.g / 255.0f,
        picker.SelectedColor.b / 255.0f,
        picker.SelectedColor.a / 255.0f,
    };

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoLabel;
    if (!picker.ShowAlpha)  flags |= ImGuiColorEditFlags_NoAlpha;
    if (!picker.ShowPicker) flags |= ImGuiColorEditFlags_NoPicker;

    bool changed = false;
    if (picker.ShowPicker)
    {
        changed = ImGui::ColorPicker4("##colorpicker", col, flags);
    }
    else
    {
        changed = ImGui::ColorEdit4("##coloredit", col, flags);
    }

    if (changed)
    {
        picker.SelectedColor = {
            static_cast<uint8_t>(col[0] * 255.0f),
            static_cast<uint8_t>(col[1] * 255.0f),
            static_cast<uint8_t>(col[2] * 255.0f),
            static_cast<uint8_t>(col[3] * 255.0f),
        };
    }
    return changed;
}

} // namespace Chained

#include "CGUIHelpers.h"
#include <cstdio>
#include <algorithm>

bool CGUISliderFloat(const char* label, float* value, float min, float max,
                     float labelWidth, float sliderWidth, const char* format, const ImVec4& sliderColor)
{
    bool changed = false;
    
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", label);
    ImGui::SameLine(labelWidth);
    
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, sliderColor);
    ImGui::SetNextItemWidth(sliderWidth);
    if (ImGui::SliderFloat((std::string("##").append(label)).c_str(), value, min, max, format)) {
        changed = true;
    }
    ImGui::PopStyleColor();
    
    // Value input
    ImGui::SameLine(labelWidth + sliderWidth + 10.0f);
    ImGui::SetNextItemWidth(80.0f);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), format, *value);
    if (ImGui::InputText((std::string("##").append(label).append("_value")).c_str(), buffer, sizeof(buffer),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal)) {
        float newValue = std::stof(buffer);
        *value = std::max(min, std::min(max, newValue));
        changed = true;
    }
    
    return changed;
}

bool CGUIVolumeSlider(const char* label, float* value, float labelWidth, float sliderWidth, const ImVec4& sliderColor)
{
    bool changed = false;
    
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", label);
    ImGui::SameLine(labelWidth);
    
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, sliderColor);
    ImGui::SetNextItemWidth(sliderWidth);
    if (ImGui::SliderFloat((std::string("##").append(label)).c_str(), value, 0.0f, 1.0f, "")) {
        changed = true;
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine(labelWidth + sliderWidth + 10.0f);
    ImGui::SetNextItemWidth(80.0f);
    int percent = static_cast<int>(*value * 100);
    if (ImGui::InputInt((std::string("##").append(label).append("_value")).c_str(), &percent, 0, 0,
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
        *value = std::max(0.0f, std::min(1.0f, percent / 100.0f));
        changed = true;
    }
    ImGui::SameLine();
    ImGui::Text("%%");
    
    return changed;
}

bool CGUICheckbox(const char* label, bool* value, float labelWidth)
{
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", label);
    ImGui::SameLine(labelWidth);
    return ImGui::Checkbox((std::string("##").append(label)).c_str(), value);
}

bool CGUIComboBox(const char* label, int* currentIndex, const std::vector<std::string>& options,
                  float labelWidth, float comboWidth)
{
    if (options.empty()) return false;
    
    if (*currentIndex < 0 || *currentIndex >= static_cast<int>(options.size())) {
        *currentIndex = 0;
    }
    
    bool changed = false;
    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "%s", label);
    ImGui::SameLine(labelWidth);
    ImGui::SetNextItemWidth(comboWidth);
    
    const char* currentValue = options[*currentIndex].c_str();
    if (ImGui::BeginCombo((std::string("##").append(label)).c_str(), currentValue)) {
        for (size_t i = 0; i < options.size(); ++i) {
            bool isSelected = (*currentIndex == static_cast<int>(i));
            if (ImGui::Selectable(options[i].c_str(), isSelected)) {
                *currentIndex = static_cast<int>(i);
                changed = true;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    return changed;
}

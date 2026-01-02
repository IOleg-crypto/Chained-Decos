#ifndef CD_SCENE_GUI_HELPERS_CGUIHELPERS_H
#define CD_SCENE_GUI_HELPERS_CGUIHELPERS_H

#include <imgui.h>
#include <vector>
#include <string>

bool CGUISliderFloat(const char* label, float* value, float min, float max, 
                     float labelWidth = 180.0f, float sliderWidth = 300.0f,
                     const char* format = "%.1f", const ImVec4& sliderColor = ImVec4(1.0f, 0.8f, 0.4f, 1.0f));

bool CGUIVolumeSlider(const char* label, float* value,
                      float labelWidth = 180.0f, float sliderWidth = 300.0f,
                      const ImVec4& sliderColor = ImVec4(0.8f, 0.6f, 1.0f, 1.0f));

bool CGUICheckbox(const char* label, bool* value, float labelWidth = 180.0f);

bool CGUIComboBox(const char* label, int* currentIndex, const std::vector<std::string>& options,
                  float labelWidth = 180.0f, float comboWidth = 200.0f);

#endif // CD_SCENE_GUI_HELPERS_CGUIHELPERS_H





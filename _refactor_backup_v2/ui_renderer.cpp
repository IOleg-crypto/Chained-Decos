#include "ui_renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/core/profiler.h"
#include <algorithm>
#include <map>

namespace CHEngine
{
    static ImVec4 ColorToImVec4(Color color)
    {
        return { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
    }

    struct UIRenderer::UIStyleScope
    {
        int ColorPushCount = 0;
        int VarPushCount = 0;
        bool Disabled = false;

        UIStyleScope() {}

        UIStyleScope(const UIStyle& style, bool interactable = true)
        {
            PushStyle(style, interactable);
        }

        UIStyleScope(const TextStyle& text, bool interactable = true)
        {
            PushText(text);
            if (!interactable)
            {
                ImGui::BeginDisabled(true);
                Disabled = true;
            }
        }

        void PushStyle(const UIStyle& style, bool interactable = true)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ColorToImVec4(style.BackgroundColor));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ColorToImVec4(style.HoverColor));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ColorToImVec4(style.PressedColor));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ColorToImVec4(style.BackgroundColor));
            ImGui::PushStyleColor(ImGuiCol_Border, ColorToImVec4(style.BorderColor));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorToImVec4(style.BackgroundColor));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ColorToImVec4(style.HoverColor));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ColorToImVec4(style.PressedColor));
            ColorPushCount += 8;

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.Rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, style.Rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.BorderSize);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.Padding, style.Padding));
            VarPushCount += 4;

            if (!interactable && !Disabled)
            {
                ImGui::BeginDisabled(true);
                Disabled = true;
            }
        }

        void PushText(const TextStyle& text)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ColorToImVec4(text.TextColor));
            ColorPushCount++;

            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{
                text.HorizontalAlignment == TextAlignment::Left ? 0.0f : (text.HorizontalAlignment == TextAlignment::Center ? 0.5f : 1.0f),
                text.VerticalAlignment == TextAlignment::Top ? 0.0f : (text.VerticalAlignment == TextAlignment::Center ? 0.5f : 1.0f)
            });
            VarPushCount++;
        }

        ~UIStyleScope()
        {
            if (Disabled) ImGui::EndDisabled();
            ImGui::PopStyleVar(VarPushCount);
            ImGui::PopStyleColor(ColorPushCount);
        }
    };

    void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
    {
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        ImVec2 currentReferenceSize = referenceSize;
        if (currentReferenceSize.x <= 0 || currentReferenceSize.y <= 0)
            currentReferenceSize = ImGui::GetIO().DisplaySize;

        auto& registry = scene->GetRegistry();
        auto uiView = registry.view<ControlComponent>();
        
        // Process UI elements by ZOrder
        std::vector<entt::entity> sortedList;
        sortedList.reserve(uiView.size());
        for (auto entityID : uiView) sortedList.push_back(entityID);

        // Sort by ZOrder
        std::sort(sortedList.begin(), sortedList.end(), [&](entt::entity a, entt::entity b) {
            return uiView.get<ControlComponent>(a).ZOrder < uiView.get<ControlComponent>(b).ZOrder;
        });

        // Map to store world-space rects for hierarchical calculation
        std::map<entt::entity, Rectangle> finalRects;

        for (entt::entity entityID : sortedList)
        {
            Entity entity{entityID, scene};
            auto &controlComponent = uiView.get<ControlComponent>(entityID);
            if (!controlComponent.IsActive) continue;

            if (entity.HasComponent<ButtonControl>())
                entity.GetComponent<ButtonControl>().PressedThisFrame = false;

            // --- 1. Hierarchical Rect Calculation ---
            Rectangle parentRect = { referencePosition.x, referencePosition.y, currentReferenceSize.x, currentReferenceSize.y };
            
            if (entity.HasComponent<HierarchyComponent>())
            {
                auto& hc = entity.GetComponent<HierarchyComponent>();
                if (hc.Parent != entt::null && finalRects.count(hc.Parent))
                {
                    parentRect = finalRects[hc.Parent];
                }
            }

            Rectangle rect = controlComponent.Transform.CalculateRect(
                {parentRect.width, parentRect.height},
                {parentRect.x, parentRect.y});
            
            finalRects[entityID] = rect;

            // Use Screen Coordinates for absolute overlay positioning
            ImVec2 screenPosition = {rect.x, rect.y};
            ImVec2 size = {rect.width, rect.height};

            ImGui::SetCursorScreenPos(screenPosition);
            ImGui::BeginGroup();
            ImGui::PushID((int)entityID);
            
            bool itemHandled = false;

            // Render Panel
            if (entity.HasComponent<PanelControl>())
            {
                auto& panel = entity.GetComponent<PanelControl>();
                UIStyleScope style(panel.Style);
                
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                
                // Draw texture if available
                if (panel.Texture && panel.Texture->IsReady())
                {
                    Texture2D& tex = panel.Texture->GetTexture();
                    ImTextureID texID = (ImTextureID)(intptr_t)tex.id;
                    
                    drawList->AddImageRounded(
                        texID,
                        screenPosition,
                        {screenPosition.x + size.x, screenPosition.y + size.y},
                        {0, 0}, {1, 1},
                        IM_COL32_WHITE,
                        panel.Style.Rounding
                    );
                }
                else
                {
                    // Fallback to solid color if texture not ready
                    drawList->AddRectFilled(screenPosition, {screenPosition.x + size.x, screenPosition.y + size.y}, 
                        ImGui::GetColorU32(ImGuiCol_ChildBg), panel.Style.Rounding);
                }
                
                // Border
                if (panel.Style.BorderSize > 0.0f)
                {
                    drawList->AddRect(screenPosition, {screenPosition.x + size.x, screenPosition.y + size.y}, 
                        ImGui::GetColorU32(ImGuiCol_Border), panel.Style.Rounding, 0, panel.Style.BorderSize);
                }
            }

            // Render Label
            if (entity.HasComponent<LabelControl>())
            {
                auto& label = entity.GetComponent<LabelControl>();
                UIStyleScope style(label.Style);
                style.PushText(label.Style);

                ImGui::PushTextWrapPos(screenPosition.x + size.x);
                ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str(), nullptr, true, size.x);
                
                float startX = 0; 
                if (label.Style.HorizontalAlignment == TextAlignment::Center) startX += (size.x - textSize.x) * 0.5f;
                else if (label.Style.HorizontalAlignment == TextAlignment::Right) startX += (size.x - textSize.x);

                float startY = 0;
                if (label.Style.VerticalAlignment == TextAlignment::Center) startY += (size.y - textSize.y) * 0.5f;
                else if (label.Style.VerticalAlignment == TextAlignment::Bottom) startY += (size.y - textSize.y);

                ImVec2 currentCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos({currentCursor.x + startX, currentCursor.y + startY});
                
                ImGui::TextUnformatted(label.Text.c_str());
                ImGui::PopTextWrapPos();
            }

            // Render Button
            if (entity.HasComponent<ButtonControl>())
            {
                auto& button = entity.GetComponent<ButtonControl>();
                UIStyleScope style(button.Style, button.IsInteractable);
                style.PushText(button.Text);
                
                if (ImGui::Button(button.Label.c_str(), size)) button.PressedThisFrame = true;
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render Slider
            if (entity.HasComponent<SliderControl>())
            {
                auto& slider = entity.GetComponent<SliderControl>();
                UIStyleScope style(slider.Style);
                style.PushText(slider.Text);

                ImGui::SetNextItemWidth(size.x);
                slider.Changed = ImGui::SliderFloat(slider.Label.c_str(), &slider.Value, slider.Min, slider.Max);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render Checkbox
            if (entity.HasComponent<CheckboxControl>())
            {
                auto& checkbox = entity.GetComponent<CheckboxControl>();
                UIStyleScope style(checkbox.Style);
                style.PushText(checkbox.Text);

                checkbox.Changed = ImGui::Checkbox(checkbox.Label.c_str(), &checkbox.Checked);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render InputText
            if (entity.HasComponent<InputTextControl>())
            {
                auto& inputText = entity.GetComponent<InputTextControl>();
                UIStyleScope style(inputText.BoxStyle, !inputText.ReadOnly);
                style.PushText(inputText.Style);

                static std::map<entt::entity, std::vector<char>> buffers;
                auto& buffer = buffers[entityID];
                if (buffer.empty() || buffer.size() != (size_t)inputText.MaxLength + 1)
                {
                    buffer.resize(inputText.MaxLength + 1, '\0');
                    if (!inputText.Text.empty())
                        strncpy(buffer.data(), inputText.Text.c_str(), inputText.MaxLength);
                }

                ImGuiInputTextFlags flags = 0;
                if (inputText.ReadOnly) flags |= ImGuiInputTextFlags_ReadOnly;
                if (inputText.Password) flags |= ImGuiInputTextFlags_Password;

                if (inputText.Multiline)
                {
                    if (ImGui::InputTextMultiline(inputText.Label.c_str(), buffer.data(), buffer.size(), size, flags))
                    {
                        inputText.Text = buffer.data();
                        inputText.Changed = true;
                    }
                } else {
                    ImGui::SetNextItemWidth(size.x);
                    if (ImGui::InputText(inputText.Label.c_str(), buffer.data(), buffer.size(), flags))
                    {
                        inputText.Text = buffer.data();
                        inputText.Changed = true;
                    }
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render ComboBox
            if (entity.HasComponent<ComboBoxControl>())
            {
                auto& comboBox = entity.GetComponent<ComboBoxControl>();
                UIStyleScope style(comboBox.BoxStyle);
                style.PushText(comboBox.Style);

                ImGui::SetNextItemWidth(size.x);
                const char* preview = comboBox.SelectedIndex >= 0 && comboBox.SelectedIndex < (int)comboBox.Items.size() 
                    ? comboBox.Items[comboBox.SelectedIndex].c_str() 
                    : "";
                
                if (ImGui::BeginCombo(comboBox.Label.c_str(), preview))
                {
                    for (int i = 0; i < (int)comboBox.Items.size(); i++)
                    {
                        bool isSelected = (i == comboBox.SelectedIndex);
                        if (ImGui::Selectable(comboBox.Items[i].c_str(), isSelected))
                        {
                            comboBox.SelectedIndex = i;
                            comboBox.Changed = true;
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render ProgressBar
            if (entity.HasComponent<ProgressBarControl>())
            {
                auto& progressBar = entity.GetComponent<ProgressBarControl>();
                UIStyleScope style(progressBar.BarStyle);
                style.PushText(progressBar.Style);

                std::string overlayText = progressBar.OverlayText;
                if (overlayText.empty() && progressBar.ShowPercentage)
                {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%.0f%%", progressBar.Progress * 100.0f);
                    overlayText = buffer;
                }

                ImGui::ProgressBar(progressBar.Progress, size, overlayText.c_str());
            }

            // Render Image
            if (entity.HasComponent<ImageControl>())
            {
                auto& image = entity.GetComponent<ImageControl>();
                UIStyleScope style(image.Style);

                auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;
                if (assetManager && !image.TexturePath.empty())
                {
                    auto textureAsset = assetManager->Get<TextureAsset>(image.TexturePath);
                    if (textureAsset)
                    {
                        Texture2D& texture = textureAsset->GetTexture();
                        ImGui::Image((ImTextureID)(intptr_t)texture.id, 
                            size, 
                            {0, 0}, {1, 1}, 
                            ColorToImVec4(image.TintColor),
                            ColorToImVec4(image.BorderColor));
                    }
                }
            }

            // Render ImageButton
            if (entity.HasComponent<ImageButtonControl>())
            {
                auto& imageButton = entity.GetComponent<ImageButtonControl>();
                UIStyleScope style(imageButton.Style);

                auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;
                if (assetManager && !imageButton.TexturePath.empty())
                {
                    auto textureAsset = assetManager->Get<TextureAsset>(imageButton.TexturePath);
                    if (textureAsset)
                    {
                        Texture2D& texture = textureAsset->GetTexture();
                        if (ImGui::ImageButton(imageButton.Label.c_str(), (ImTextureID)(intptr_t)texture.id, 
                            size, 
                            {0, 0}, {1, 1}, 
                            ColorToImVec4(imageButton.BackgroundColor),
                            ColorToImVec4(imageButton.TintColor)))
                        {
                            imageButton.PressedThisFrame = true;
                        }
                        if (ImGui::IsItemActive()) itemHandled = true;
                    }
                }
            }

            // Render Separator
            if (entity.HasComponent<SeparatorControl>())
            {
                auto& separator = entity.GetComponent<SeparatorControl>();
                ImGui::PushStyleColor(ImGuiCol_Separator, ColorToImVec4(separator.LineColor));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            // Render RadioButton
            if (entity.HasComponent<RadioButtonControl>())
            {
                auto& radioButton = entity.GetComponent<RadioButtonControl>();
                UIStyleScope style(radioButton.Style);

                for (int i = 0; i < (int)radioButton.Options.size(); i++)
                {
                    if (ImGui::RadioButton(radioButton.Options[i].c_str(), radioButton.SelectedIndex == i))
                    {
                        radioButton.SelectedIndex = i;
                        radioButton.Changed = true;
                    }
                    if (radioButton.Horizontal && i < (int)radioButton.Options.size() - 1) ImGui::SameLine();
                    if (ImGui::IsItemActive()) itemHandled = true;
                }
            }

            // Render ColorPicker
            if (entity.HasComponent<ColorPickerControl>())
            {
                auto& colorPicker = entity.GetComponent<ColorPickerControl>();
                UIStyleScope style(colorPicker.Style);

                float color[4] = { colorPicker.SelectedColor.r / 255.0f, colorPicker.SelectedColor.g / 255.0f, colorPicker.SelectedColor.b / 255.0f, colorPicker.SelectedColor.a / 255.0f };
                ImGuiColorEditFlags flags = colorPicker.ShowAlpha ? ImGuiColorEditFlags_AlphaBar : 0;
                
                if (colorPicker.ShowPicker)
                    colorPicker.Changed = ImGui::ColorPicker4(colorPicker.Label.c_str(), color, flags);
                else
                    colorPicker.Changed = ImGui::ColorEdit4(colorPicker.Label.c_str(), color, flags);

                if (colorPicker.Changed)
                {
                    colorPicker.SelectedColor = { (unsigned char)(color[0] * 255), (unsigned char)(color[1] * 255), (unsigned char)(color[2] * 255), (unsigned char)(color[3] * 255) };
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render DragFloat
            if (entity.HasComponent<DragFloatControl>())
            {
                auto& dragFloat = entity.GetComponent<DragFloatControl>();
                UIStyleScope style(dragFloat.BoxStyle);
                style.PushText(dragFloat.Style);

                ImGui::SetNextItemWidth(size.x);
                dragFloat.Changed = ImGui::DragFloat(dragFloat.Label.c_str(), &dragFloat.Value, dragFloat.Speed, dragFloat.Min, dragFloat.Max, dragFloat.Format.c_str());
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render DragInt
            if (entity.HasComponent<DragIntControl>())
            {
                auto& dragInt = entity.GetComponent<DragIntControl>();
                UIStyleScope style(dragInt.BoxStyle);
                style.PushText(dragInt.Style);

                ImGui::SetNextItemWidth(size.x);
                dragInt.Changed = ImGui::DragInt(dragInt.Label.c_str(), &dragInt.Value, dragInt.Speed, dragInt.Min, dragInt.Max, dragInt.Format.c_str());
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render TreeNode
            if (entity.HasComponent<TreeNodeControl>())
            {
                auto& treeNode = entity.GetComponent<TreeNodeControl>();
                UIStyleScope style(treeNode.Style);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                if (treeNode.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
                if (treeNode.IsLeaf) flags |= ImGuiTreeNodeFlags_Leaf;
                
                treeNode.IsOpen = ImGui::TreeNodeEx(treeNode.Label.c_str(), flags);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render TabBar
            if (entity.HasComponent<TabBarControl>())
            {
                auto& tabBar = entity.GetComponent<TabBarControl>();
                UIStyleScope style(tabBar.Style);

                ImGuiTabBarFlags flags = ImGuiTabBarFlags_None;
                if (tabBar.Reorderable) flags |= ImGuiTabBarFlags_Reorderable;
                if (tabBar.AutoSelectNewTabs) flags |= ImGuiTabBarFlags_AutoSelectNewTabs;

                if (ImGui::BeginTabBar(tabBar.Label.c_str(), flags))
                {
                    ImGui::EndTabBar();
                }
            }

            // Render TabItem
            if (entity.HasComponent<TabItemControl>())
            {
                auto& tabItem = entity.GetComponent<TabItemControl>();
                UIStyleScope style(tabItem.Style);

                if (ImGui::BeginTabItem(tabItem.Label.c_str(), &tabItem.IsOpen))
                {
                    tabItem.Selected = true;
                    ImGui::EndTabItem();
                }
                else tabItem.Selected = false;
            }

            // Render CollapsingHeader
            if (entity.HasComponent<CollapsingHeaderControl>())
            {
                auto& collapsingHeader = entity.GetComponent<CollapsingHeaderControl>();
                UIStyleScope style(collapsingHeader.Style);

                ImGuiTreeNodeFlags flags = 0;
                if (collapsingHeader.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

                collapsingHeader.IsOpen = ImGui::CollapsingHeader(collapsingHeader.Label.c_str(), flags);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render PlotLines
            if (entity.HasComponent<PlotLinesControl>())
            {
                auto& plotLines = entity.GetComponent<PlotLinesControl>();
                UIStyleScope style(plotLines.BoxStyle);
                style.PushText(plotLines.Style);

                ImGui::PlotLines(plotLines.Label.c_str(), plotLines.Values.data(), (int)plotLines.Values.size(), 
                    0, plotLines.OverlayText.c_str(), plotLines.ScaleMin, plotLines.ScaleMax, {plotLines.GraphSize.x, plotLines.GraphSize.y});
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render PlotHistogram
            if (entity.HasComponent<PlotHistogramControl>())
            {
                auto& plotHistogram = entity.GetComponent<PlotHistogramControl>();
                UIStyleScope style(plotHistogram.BoxStyle);
                style.PushText(plotHistogram.Style);

                ImGui::PlotHistogram(plotHistogram.Label.c_str(), plotHistogram.Values.data(), (int)plotHistogram.Values.size(), 
                    0, plotHistogram.OverlayText.c_str(), plotHistogram.ScaleMin, plotHistogram.ScaleMax, {plotHistogram.GraphSize.x, plotHistogram.GraphSize.y});
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Drag-and-drop logic for Edit Mode
            if (editMode) {
                bool dragging = false;
                ImVec2 delta = { 0, 0 };

                if (itemHandled) {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
                        dragging = true;
                        delta = ImGui::GetIO().MouseDelta;
                    }
                } else {
                    ImGui::SetCursorScreenPos(screenPosition);
                    ImGui::InvisibleButton("##SelectionZone", size);
                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
                        dragging = true;
                        delta = ImGui::GetIO().MouseDelta;
                    }
                }

                if (dragging) {
                    controlComponent.Transform.OffsetMin.x += delta.x;
                    controlComponent.Transform.OffsetMax.x += delta.x;
                    controlComponent.Transform.OffsetMin.y += delta.y;
                    controlComponent.Transform.OffsetMax.y += delta.y;
                }
            }

            ImGui::PopID();
            ImGui::EndGroup();
        }
    }
}

#include "ui_renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/entity.h"
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

    void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& refPos, const ImVec2& refSize, bool editMode)
    {
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        ImVec2 referenceSize = refSize;
        if (referenceSize.x <= 0 || referenceSize.y <= 0)
            referenceSize = ImGui::GetIO().DisplaySize;

        auto& registry = scene->GetRegistry();
        auto uiView = registry.view<ControlComponent>();
        
        CH_CORE_TRACE("DrawUI: UI Entities={0}, RefPos=({1},{2}), RefSize=({3},{4})", 
            uiView.size(), refPos.x, refPos.y, referenceSize.x, referenceSize.y);

        // Process UI elements by ZOrder
        std::vector<entt::entity> sortedList;
        sortedList.reserve(uiView.size());
        for (auto entityID : uiView) sortedList.push_back(entityID);

        // Sort by ZOrder
        std::sort(sortedList.begin(), sortedList.end(), [&](entt::entity a, entt::entity b) {
            return uiView.get<ControlComponent>(a).ZOrder < uiView.get<ControlComponent>(b).ZOrder;
        });

        for (entt::entity entityID : sortedList)
        {
            Entity entity{entityID, scene};
            auto &cc = uiView.get<ControlComponent>(entityID);
            if (!cc.IsActive) continue;

            if (entity.HasComponent<ButtonControl>())
                entity.GetComponent<ButtonControl>().PressedThisFrame = false;

            auto rect = cc.Transform.CalculateRect(
                {referenceSize.x, referenceSize.y},
                {refPos.x, refPos.y});
            
            // Use Screen Coordinates for absolute overlay positioning
            ImVec2 screenPos = {rect.x, rect.y};
            ImVec2 size = {rect.width, rect.height};

            ImGui::SetCursorScreenPos(screenPos);
            ImGui::BeginGroup();
            ImGui::PushID((int)entityID);
            
            bool itemHandled = false;

            // Render Panel
            if (entity.HasComponent<PanelControl>())
            {
                auto& pnl = entity.GetComponent<PanelControl>();
                UIStyleScope style(pnl.Style);
                
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(screenPos, {screenPos.x + size.x, screenPos.y + size.y}, 
                    ImGui::GetColorU32(ImGuiCol_ChildBg), pnl.Style.Rounding);
                
                if (pnl.Style.BorderSize > 0.0f)
                {
                    drawList->AddRect(screenPos, {screenPos.x + size.x, screenPos.y + size.y}, 
                        ImGui::GetColorU32(ImGuiCol_Border), pnl.Style.Rounding, 0, pnl.Style.BorderSize);
                }
            }

            // Render Label
            if (entity.HasComponent<LabelControl>())
            {
                auto& lbl = entity.GetComponent<LabelControl>();
                UIStyleScope style(lbl.Style);
                style.PushText(lbl.Style);

                ImGui::PushTextWrapPos(screenPos.x + size.x);
                ImVec2 textSize = ImGui::CalcTextSize(lbl.Text.c_str(), nullptr, true, size.x);
                
                float startX = 0; 
                if (lbl.Style.HorizontalAlignment == TextAlignment::Center) startX += (size.x - textSize.x) * 0.5f;
                else if (lbl.Style.HorizontalAlignment == TextAlignment::Right) startX += (size.x - textSize.x);

                float startY = 0;
                if (lbl.Style.VerticalAlignment == TextAlignment::Center) startY += (size.y - textSize.y) * 0.5f;
                else if (lbl.Style.VerticalAlignment == TextAlignment::Bottom) startY += (size.y - textSize.y);

                ImVec2 currentCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos({currentCursor.x + startX, currentCursor.y + startY});
                
                ImGui::TextUnformatted(lbl.Text.c_str());
                ImGui::PopTextWrapPos();
            }

            // Render Button
            if (entity.HasComponent<ButtonControl>())
            {
                auto& btn = entity.GetComponent<ButtonControl>();
                UIStyleScope style(btn.Style, btn.IsInteractable);
                style.PushText(btn.Text);
                
                if (ImGui::Button(btn.Label.c_str(), size)) btn.PressedThisFrame = true;
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render Slider
            if (entity.HasComponent<SliderControl>())
            {
                auto& sl = entity.GetComponent<SliderControl>();
                UIStyleScope style(sl.Style);
                style.PushText(sl.Text);

                ImGui::SetNextItemWidth(size.x);
                sl.Changed = ImGui::SliderFloat(sl.Label.c_str(), &sl.Value, sl.Min, sl.Max);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render Checkbox
            if (entity.HasComponent<CheckboxControl>())
            {
                auto& cb = entity.GetComponent<CheckboxControl>();
                UIStyleScope style(cb.Style);
                style.PushText(cb.Text);

                cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render InputText
            if (entity.HasComponent<InputTextControl>())
            {
                auto& input = entity.GetComponent<InputTextControl>();
                UIStyleScope style(input.BoxStyle, !input.ReadOnly);
                style.PushText(input.Style);

                static std::map<entt::entity, std::vector<char>> buffers;
                auto& buffer = buffers[entityID];
                if (buffer.empty() || buffer.size() != input.MaxLength + 1)
                {
                    buffer.resize(input.MaxLength + 1, '\0');
                    if (!input.Text.empty())
                        strncpy(buffer.data(), input.Text.c_str(), input.MaxLength);
                }

                ImGuiInputTextFlags flags = 0;
                if (input.ReadOnly) flags |= ImGuiInputTextFlags_ReadOnly;
                if (input.Password) flags |= ImGuiInputTextFlags_Password;

                if (input.Multiline)
                {
                    if (ImGui::InputTextMultiline(input.Label.c_str(), buffer.data(), buffer.size(), size, flags))
                    {
                        input.Text = buffer.data();
                        input.Changed = true;
                    }
                } else {
                    ImGui::SetNextItemWidth(size.x);
                    if (ImGui::InputText(input.Label.c_str(), buffer.data(), buffer.size(), flags))
                    {
                        input.Text = buffer.data();
                        input.Changed = true;
                    }
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render ComboBox
            if (entity.HasComponent<ComboBoxControl>())
            {
                auto& combo = entity.GetComponent<ComboBoxControl>();
                UIStyleScope style(combo.BoxStyle);
                style.PushText(combo.Style);

                ImGui::SetNextItemWidth(size.x);
                const char* preview = combo.SelectedIndex >= 0 && combo.SelectedIndex < combo.Items.size() 
                    ? combo.Items[combo.SelectedIndex].c_str() 
                    : "";
                
                if (ImGui::BeginCombo(combo.Label.c_str(), preview))
                {
                    for (int i = 0; i < combo.Items.size(); i++)
                    {
                        bool isSelected = (i == combo.SelectedIndex);
                        if (ImGui::Selectable(combo.Items[i].c_str(), isSelected))
                        {
                            combo.SelectedIndex = i;
                            combo.Changed = true;
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
                auto& prog = entity.GetComponent<ProgressBarControl>();
                UIStyleScope style(prog.BarStyle);
                style.PushText(prog.Style);

                std::string overlayText = prog.OverlayText;
                if (overlayText.empty() && prog.ShowPercentage)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.0f%%", prog.Progress * 100.0f);
                    overlayText = buf;
                }

                ImGui::ProgressBar(prog.Progress, size, overlayText.c_str());
            }

            // Render Image
            if (entity.HasComponent<ImageControl>())
            {
                auto& img = entity.GetComponent<ImageControl>();
                UIStyleScope style(img.Style);

                auto am = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;
                if (am && !img.TexturePath.empty())
                {
                    auto texAsset = am->Get<TextureAsset>(img.TexturePath);
                    if (texAsset)
                    {
                        Texture2D& tex = texAsset->GetTexture();
                        ImGui::Image((ImTextureID)(intptr_t)tex.id, 
                            { img.Size.x, img.Size.y }, 
                            {0, 0}, {1, 1}, 
                            ColorToImVec4(img.TintColor),
                            ColorToImVec4(img.BorderColor));
                    }
                }
            }

            // Render ImageButton
            if (entity.HasComponent<ImageButtonControl>())
            {
                auto& btn = entity.GetComponent<ImageButtonControl>();
                UIStyleScope style(btn.Style);

                auto am = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;
                if (am && !btn.TexturePath.empty())
                {
                    auto texAsset = am->Get<TextureAsset>(btn.TexturePath);
                    if (texAsset)
                    {
                        Texture2D& tex = texAsset->GetTexture();
                        if (ImGui::ImageButton(btn.Label.c_str(), (ImTextureID)(intptr_t)tex.id, 
                            { btn.Size.x, btn.Size.y }, 
                            {0, 0}, {1, 1}, 
                            ColorToImVec4(btn.BackgroundColor),
                            ColorToImVec4(btn.TintColor)))
                        {
                            btn.PressedThisFrame = true;
                        }
                        if (ImGui::IsItemActive()) itemHandled = true;
                    }
                }
            }

            // Render Separator
            if (entity.HasComponent<SeparatorControl>())
            {
                auto& sep = entity.GetComponent<SeparatorControl>();
                ImGui::PushStyleColor(ImGuiCol_Separator, ColorToImVec4(sep.LineColor));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            // Render RadioButton
            if (entity.HasComponent<RadioButtonControl>())
            {
                auto& rb = entity.GetComponent<RadioButtonControl>();
                UIStyleScope style(rb.Style);

                for (int i = 0; i < rb.Options.size(); i++)
                {
                    if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i))
                    {
                        rb.SelectedIndex = i;
                        rb.Changed = true;
                    }
                    if (rb.Horizontal && i < (int)rb.Options.size() - 1) ImGui::SameLine();
                    if (ImGui::IsItemActive()) itemHandled = true;
                }
            }

            // Render ColorPicker
            if (entity.HasComponent<ColorPickerControl>())
            {
                auto& cp = entity.GetComponent<ColorPickerControl>();
                UIStyleScope style(cp.Style);

                float col[4] = { cp.SelectedColor.r / 255.0f, cp.SelectedColor.g / 255.0f, cp.SelectedColor.b / 255.0f, cp.SelectedColor.a / 255.0f };
                ImGuiColorEditFlags flags = cp.ShowAlpha ? ImGuiColorEditFlags_AlphaBar : 0;
                
                if (cp.ShowPicker)
                    cp.Changed = ImGui::ColorPicker4(cp.Label.c_str(), col, flags);
                else
                    cp.Changed = ImGui::ColorEdit4(cp.Label.c_str(), col, flags);

                if (cp.Changed)
                {
                    cp.SelectedColor = { (unsigned char)(col[0] * 255), (unsigned char)(col[1] * 255), (unsigned char)(col[2] * 255), (unsigned char)(col[3] * 255) };
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render DragFloat
            if (entity.HasComponent<DragFloatControl>())
            {
                auto& df = entity.GetComponent<DragFloatControl>();
                UIStyleScope style(df.BoxStyle);
                style.PushText(df.Style);

                ImGui::SetNextItemWidth(size.x);
                df.Changed = ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max, df.Format.c_str());
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render DragInt
            if (entity.HasComponent<DragIntControl>())
            {
                auto& di = entity.GetComponent<DragIntControl>();
                UIStyleScope style(di.BoxStyle);
                style.PushText(di.Style);

                ImGui::SetNextItemWidth(size.x);
                di.Changed = ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max, di.Format.c_str());
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render TreeNode
            if (entity.HasComponent<TreeNodeControl>())
            {
                auto& tn = entity.GetComponent<TreeNodeControl>();
                UIStyleScope style(tn.Style);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                if (tn.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;
                if (tn.IsLeaf) flags |= ImGuiTreeNodeFlags_Leaf;
                
                tn.IsOpen = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render TabBar
            if (entity.HasComponent<TabBarControl>())
            {
                auto& tb = entity.GetComponent<TabBarControl>();
                UIStyleScope style(tb.Style);

                ImGuiTabBarFlags flags = ImGuiTabBarFlags_None;
                if (tb.Reorderable) flags |= ImGuiTabBarFlags_Reorderable;
                if (tb.AutoSelectNewTabs) flags |= ImGuiTabBarFlags_AutoSelectNewTabs;

                if (ImGui::BeginTabBar(tb.Label.c_str(), flags))
                {
                    ImGui::EndTabBar();
                }
            }

            // Render TabItem
            if (entity.HasComponent<TabItemControl>())
            {
                auto& ti = entity.GetComponent<TabItemControl>();
                UIStyleScope style(ti.Style);

                if (ImGui::BeginTabItem(ti.Label.c_str(), &ti.IsOpen))
                {
                    ti.Selected = true;
                    ImGui::EndTabItem();
                }
                else ti.Selected = false;
            }

            // Render CollapsingHeader
            if (entity.HasComponent<CollapsingHeaderControl>())
            {
                auto& ch = entity.GetComponent<CollapsingHeaderControl>();
                UIStyleScope style(ch.Style);

                ImGuiTreeNodeFlags flags = 0;
                if (ch.DefaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

                ch.IsOpen = ImGui::CollapsingHeader(ch.Label.c_str(), flags);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render PlotLines
            if (entity.HasComponent<PlotLinesControl>())
            {
                auto& plot = entity.GetComponent<PlotLinesControl>();
                UIStyleScope style(plot.BoxStyle);
                style.PushText(plot.Style);

                ImGui::PlotLines(plot.Label.c_str(), plot.Values.data(), (int)plot.Values.size(), 
                    0, plot.OverlayText.c_str(), plot.ScaleMin, plot.ScaleMax, {plot.GraphSize.x, plot.GraphSize.y});
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            // Render PlotHistogram
            if (entity.HasComponent<PlotHistogramControl>())
            {
                auto& hist = entity.GetComponent<PlotHistogramControl>();
                UIStyleScope style(hist.BoxStyle);
                style.PushText(hist.Style);

                ImGui::PlotHistogram(hist.Label.c_str(), hist.Values.data(), (int)hist.Values.size(), 
                    0, hist.OverlayText.c_str(), hist.ScaleMin, hist.ScaleMax, {hist.GraphSize.x, hist.GraphSize.y});
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
                    ImGui::SetCursorScreenPos(screenPos);
                    ImGui::InvisibleButton("##SelectionZone", size);
                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
                        dragging = true;
                        delta = ImGui::GetIO().MouseDelta;
                    }
                }

                if (dragging) {
                    cc.Transform.OffsetMin.x += delta.x;
                    cc.Transform.OffsetMax.x += delta.x;
                    cc.Transform.OffsetMin.y += delta.y;
                    cc.Transform.OffsetMax.y += delta.y;
                }
            }

            ImGui::PopID();
            ImGui::EndGroup();
        }
    }
}

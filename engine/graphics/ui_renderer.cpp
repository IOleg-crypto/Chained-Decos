#include "ui_renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/font_asset.h"
#include "engine/core/profiler.h"
#include <algorithm>
#include <map>

namespace CHEngine
{
    UIRenderer* UIRenderer::s_Instance = nullptr;

    void UIRenderer::Init()
    {
        CH_CORE_ASSERT(!s_Instance, "UIRenderer already initialized!");
        s_Instance = new UIRenderer();
        CH_CORE_INFO("Initializing UIRenderer...");
    }

    void UIRenderer::Shutdown()
    {
        CH_CORE_INFO("Shutting down UIRenderer...");
        delete s_Instance;
        s_Instance = nullptr;
    }

    UIRenderer::UIRenderer()
    {
        m_Data = std::make_unique<UIRendererData>();
    }

    UIRenderer::~UIRenderer()
    {
    }

    static ImVec4 ColorToImVec4(Color color)
    {
        return { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
    }

    Rectangle UIRenderer::GetEntityRect(Entity entity, const ImVec2& viewportSize, const ImVec2& viewportOffset)
    {
        if (!entity || !entity.HasComponent<ControlComponent>()) return {0,0,0,0};

        auto& control = entity.GetComponent<ControlComponent>();
        Rectangle parentRect = { viewportOffset.x, viewportOffset.y, viewportSize.x, viewportSize.y };

        if (entity.HasComponent<HierarchyComponent>())
        {
            auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
            if (parentID != entt::null)
            {
                Entity parent{parentID, &entity.GetRegistry()};
                if (parent.HasComponent<ControlComponent>())
                    parentRect = GetEntityRect(parent, viewportSize, viewportOffset);
            }
        }

        return control.Transform.CalculateRect({parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
    }

    UIRenderer::UIStyleScope::~UIStyleScope()
    {
        if (Disabled) ImGui::EndDisabled();
        while (FontPushCount > 0) { ImGui::PopFont(); FontPushCount--; }
        if (OldFontScale != 1.0f) ImGui::SetWindowFontScale(OldFontScale);
        ImGui::PopStyleVar(VarPushCount);
        ImGui::PopStyleColor(ColorPushCount);
    }

    void UIRenderer::UIStyleScope::PushStyle(const UIStyle& style, bool interactable)
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

        if (!interactable)
        {
            ImGui::BeginDisabled(true);
            Disabled = true;
        }
    }

    void UIRenderer::UIStyleScope::PushText(const TextStyle& text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ColorToImVec4(text.TextColor));
        ColorPushCount++;

        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{
            text.HorizontalAlignment == TextAlignment::Left ? 0.0f : (text.HorizontalAlignment == TextAlignment::Center ? 0.5f : 1.0f),
            text.VerticalAlignment == TextAlignment::Top ? 0.0f : (text.VerticalAlignment == TextAlignment::Center ? 0.5f : 1.0f)
        });
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

        if (fontName.empty() || fontName == "Default") return;

        auto project = Project::GetActive();
        if (!project) return;

        auto am = project->GetAssetManager();
        auto fontAsset = am->Get<FontAsset>(fontName);
        
        // Note: For real ImGui font switching, we need to ensure the font is loaded into ImGui atlas.
        // For now we rely on SetWindowFontScale as requested.
    }

    // --- Modular Rendering Helpers ---

    static void DrawPanel(const PanelControl& panel, const ImVec2& pos, const ImVec2& size)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_ChildBg);
        ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);

        if (panel.Texture && panel.Texture->IsReady())
        {
            drawList->AddImageRounded((ImTextureID)(intptr_t)panel.Texture->GetTexture().id,
                pos, {pos.x + size.x, pos.y + size.y}, {0,0}, {1,1}, IM_COL32_WHITE, panel.Style.Rounding);
        }
        else
        {
            drawList->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, bgColor, panel.Style.Rounding);
        }

        if (panel.Style.BorderSize > 0.0f)
            drawList->AddRect(pos, {pos.x + size.x, pos.y + size.y}, borderColor, panel.Style.Rounding, 0, panel.Style.BorderSize);
    }

    static void DrawLabel(const LabelControl& label, const ImVec2& size)
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
        ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str(), nullptr, true, size.x);
        
        float startX = 0, startY = 0;
        if (label.Style.HorizontalAlignment == TextAlignment::Center) startX = (size.x - textSize.x) * 0.5f;
        else if (label.Style.HorizontalAlignment == TextAlignment::Right) startX = (size.x - textSize.x);

        if (label.Style.VerticalAlignment == TextAlignment::Center) startY = (size.y - textSize.y) * 0.5f;
        else if (label.Style.VerticalAlignment == TextAlignment::Bottom) startY = (size.y - textSize.y);

        ImGui::SetCursorPos({ImGui::GetCursorPosX() + startX, ImGui::GetCursorPosY() + startY});
        ImGui::TextUnformatted(label.Text.c_str());
        ImGui::PopTextWrapPos();
    }

    void UIRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize, bool editMode)
    {
        CH_CORE_ASSERT(scene, "Scene is null!");
        CH_PROFILE_FUNCTION();

        ImVec2 currentRefSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
        auto& registry = scene->GetRegistry();
        auto uiView = registry.view<ControlComponent>();
        
        std::vector<entt::entity> sortedEntities;
        for (auto entityID : uiView) sortedEntities.push_back(entityID);

        std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
            return uiView.get<ControlComponent>(a).ZOrder < uiView.get<ControlComponent>(b).ZOrder;
        });

        std::map<entt::entity, Rectangle> rectCache;

        for (entt::entity id : sortedEntities)
        {
            Entity entity(id, &scene->GetRegistry());
            auto& control = uiView.get<ControlComponent>(id);
            if (!control.IsActive) continue;

            // Reset frame flags
            if (entity.HasComponent<ButtonControl>()) entity.GetComponent<ButtonControl>().PressedThisFrame = false;

            // 1. Calculate Rect
            Rectangle parentRect = { referencePosition.x, referencePosition.y, currentRefSize.x, currentRefSize.y };
            if (entity.HasComponent<HierarchyComponent>())
            {
                auto parent = entity.GetComponent<HierarchyComponent>().Parent;
                if (parent != entt::null && rectCache.count(parent)) parentRect = rectCache[parent];
            }

            // --- AutoSize Calculation (Pre-rendering) ---
            if (entity.HasComponent<LabelControl>() && entity.GetComponent<LabelControl>().AutoSize)
            {
                auto& label = entity.GetComponent<LabelControl>();
                ImVec2 textSize = ImGui::CalcTextSize(label.Text.c_str());
                control.Transform.OffsetMax = { control.Transform.OffsetMin.x + textSize.x + 10.0f, control.Transform.OffsetMin.y + textSize.y + 4.0f };
            }
            else if (entity.HasComponent<ButtonControl>() && entity.GetComponent<ButtonControl>().AutoSize)
            {
                auto& button = entity.GetComponent<ButtonControl>();
                ImVec2 textSize = ImGui::CalcTextSize(button.Label.c_str());
                float pad = button.Style.Padding * 2.0f;
                control.Transform.OffsetMax = { control.Transform.OffsetMin.x + textSize.x + pad + 10.0f, control.Transform.OffsetMin.y + textSize.y + pad + 4.0f };
            }

            Rectangle rect = control.Transform.CalculateRect({parentRect.width, parentRect.height}, {parentRect.x, parentRect.y});
            rectCache[id] = rect;

            ImVec2 screenPos = {rect.x, rect.y};
            ImVec2 size = {rect.width, rect.height};

            ImGui::SetCursorScreenPos(screenPos);
            ImGui::BeginGroup(); // Group for interaction and layout
            ImGui::PushID((int)id);
            
            bool itemHandled = false;

            // 2. Specialized Rendering
            if (entity.HasComponent<PanelControl>()) DrawPanel(entity.GetComponent<PanelControl>(), screenPos, size);

            if (entity.HasComponent<LabelControl>())
            {
                auto& label = entity.GetComponent<LabelControl>();
                UIStyleScope scope; 
                scope.PushText(label.Style);
                DrawLabel(label, size);
            }

            if (entity.HasComponent<ButtonControl>())
            {
                auto& button = entity.GetComponent<ButtonControl>();
                UIStyleScope scope;
                scope.PushStyle(button.Style, button.IsInteractable);
                scope.PushText(button.Text);
                if (ImGui::Button(button.Label.c_str(), size)) button.PressedThisFrame = true;
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<SliderControl>())
            {
                auto& slider = entity.GetComponent<SliderControl>();
                UIStyleScope scope; scope.PushStyle(slider.Style); scope.PushText(slider.Text);
                ImGui::SetNextItemWidth(size.x);
                slider.Changed = ImGui::SliderFloat(slider.Label.c_str(), &slider.Value, slider.Min, slider.Max);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<CheckboxControl>())
            {
                auto& cb = entity.GetComponent<CheckboxControl>();
                UIStyleScope scope; scope.PushStyle(cb.Style); scope.PushText(cb.Text);
                cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<ImageControl>())
            {
                auto& image = entity.GetComponent<ImageControl>();
                auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;
                if (assetManager && !image.TexturePath.empty())
                {
                    auto texAsset = assetManager->Get<TextureAsset>(image.TexturePath);
                    if (texAsset)
                    {
                        ImGui::Image((ImTextureID)(intptr_t)texAsset->GetTexture().id, size, {0,0}, {1,1}, 
                            ColorToImVec4(image.TintColor), ColorToImVec4(image.BorderColor));
                    }
                }
            }

            if (entity.HasComponent<InputTextControl>())
            {
                auto& it = entity.GetComponent<InputTextControl>();
                UIStyleScope scope; scope.PushStyle(it.BoxStyle, !it.ReadOnly); scope.PushText(it.Style);
                
                auto& buffer = m_Data->InputBuffers[id];
                if (buffer.size() != (size_t)it.MaxLength + 1)
                {
                    buffer.resize(it.MaxLength + 1, '\0');
                    strncpy(buffer.data(), it.Text.c_str(), it.MaxLength);
                }

                ImGuiInputTextFlags flags = (it.ReadOnly ? ImGuiInputTextFlags_ReadOnly : 0) | (it.Password ? ImGuiInputTextFlags_Password : 0);
                if (it.Multiline) {
                    if (ImGui::InputTextMultiline(it.Label.c_str(), buffer.data(), buffer.size(), size, flags))
                    { it.Text = buffer.data(); it.Changed = true; }
                } else {
                    ImGui::SetNextItemWidth(size.x);
                    if (ImGui::InputText(it.Label.c_str(), buffer.data(), buffer.size(), flags))
                    { it.Text = buffer.data(); it.Changed = true; }
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<ProgressBarControl>())
            {
                auto& pb = entity.GetComponent<ProgressBarControl>();
                UIStyleScope scope; scope.PushStyle(pb.BarStyle); scope.PushText(pb.Style);
                std::string overlay = pb.OverlayText;
                if (overlay.empty() && pb.ShowPercentage) overlay = std::to_string((int)(pb.Progress * 100)) + "%";
                ImGui::ProgressBar(pb.Progress, size, overlay.c_str());
            }

            if (entity.HasComponent<ComboBoxControl>())
            {
                auto& cb = entity.GetComponent<ComboBoxControl>();
                UIStyleScope scope; scope.PushStyle(cb.BoxStyle); scope.PushText(cb.Style);
                ImGui::SetNextItemWidth(size.x);
                const char* preview = (cb.SelectedIndex >= 0 && cb.SelectedIndex < (int)cb.Items.size()) ? cb.Items[cb.SelectedIndex].c_str() : "";
                if (ImGui::BeginCombo(cb.Label.c_str(), preview)) {
                    for (int i = 0; i < (int)cb.Items.size(); i++) {
                        if (ImGui::Selectable(cb.Items[i].c_str(), i == cb.SelectedIndex)) { cb.SelectedIndex = i; cb.Changed = true; }
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<ImageButtonControl>())
            {
                auto& ib = entity.GetComponent<ImageButtonControl>();
                auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;
                if (assetManager && !ib.TexturePath.empty()) {
                    auto tex = assetManager->Get<TextureAsset>(ib.TexturePath);
                    if (tex) {
                        if (ImGui::ImageButton(ib.Label.c_str(), (ImTextureID)(intptr_t)tex->GetTexture().id, size, {0,0}, {1,1}, 
                            ColorToImVec4(ib.BackgroundColor), ColorToImVec4(ib.TintColor))) ib.PressedThisFrame = true;
                        if (ImGui::IsItemActive()) itemHandled = true;
                    }
                }
            }

            if (entity.HasComponent<RadioButtonControl>())
            {
                auto& rb = entity.GetComponent<RadioButtonControl>();
                UIStyleScope scope; scope.PushText(rb.Style);
                for (int i = 0; i < (int)rb.Options.size(); i++) {
                    if (ImGui::RadioButton(rb.Options[i].c_str(), rb.SelectedIndex == i)) { rb.SelectedIndex = i; rb.Changed = true; }
                    if (rb.Horizontal && i < (int)rb.Options.size() - 1) ImGui::SameLine();
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<ColorPickerControl>())
            {
                auto& cp = entity.GetComponent<ColorPickerControl>();
                float c[4] = { cp.SelectedColor.r/255.f, cp.SelectedColor.g/255.f, cp.SelectedColor.b/255.f, cp.SelectedColor.a/255.f };
                if (cp.ShowPicker ? ImGui::ColorPicker4(cp.Label.c_str(), c) : ImGui::ColorEdit4(cp.Label.c_str(), c)) {
                    cp.SelectedColor = { (uint8_t)(c[0]*255), (uint8_t)(c[1]*255), (uint8_t)(c[2]*255), (uint8_t)(c[3]*255) };
                    cp.Changed = true;
                }
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<SeparatorControl>())
            {
                auto& sep = entity.GetComponent<SeparatorControl>();
                ImGui::PushStyleColor(ImGuiCol_Separator, ColorToImVec4(sep.LineColor));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            if (entity.HasComponent<DragFloatControl>())
            {
                auto& df = entity.GetComponent<DragFloatControl>();
                UIStyleScope scope; scope.PushStyle(df.BoxStyle); scope.PushText(df.Style);
                ImGui::SetNextItemWidth(size.x);
                df.Changed = ImGui::DragFloat(df.Label.c_str(), &df.Value, df.Speed, df.Min, df.Max, df.Format.c_str());
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<DragIntControl>())
            {
                auto& di = entity.GetComponent<DragIntControl>();
                UIStyleScope scope; scope.PushStyle(di.BoxStyle); scope.PushText(di.Style);
                ImGui::SetNextItemWidth(size.x);
                di.Changed = ImGui::DragInt(di.Label.c_str(), &di.Value, di.Speed, di.Min, di.Max, di.Format.c_str());
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<TreeNodeControl>())
            {
                auto& tn = entity.GetComponent<TreeNodeControl>();
                ImGuiTreeNodeFlags flags = (tn.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0) | (tn.IsLeaf ? ImGuiTreeNodeFlags_Leaf : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
                tn.IsOpen = ImGui::TreeNodeEx(tn.Label.c_str(), flags);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<CollapsingHeaderControl>())
            {
                auto& ch = entity.GetComponent<CollapsingHeaderControl>();
                ch.IsOpen = ImGui::CollapsingHeader(ch.Label.c_str(), ch.DefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<PlotLinesControl>())
            {
                auto& pl = entity.GetComponent<PlotLinesControl>();
                UIStyleScope scope; scope.PushStyle(pl.BoxStyle); scope.PushText(pl.Style);
                ImGui::PlotLines(pl.Label.c_str(), pl.Values.data(), (int)pl.Values.size(), 0, pl.OverlayText.c_str(), pl.ScaleMin, pl.ScaleMax, {pl.GraphSize.x, pl.GraphSize.y});
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<PlotHistogramControl>())
            {
                auto& ph = entity.GetComponent<PlotHistogramControl>();
                UIStyleScope scope; scope.PushStyle(ph.BoxStyle); scope.PushText(ph.Style);
                ImGui::PlotHistogram(ph.Label.c_str(), ph.Values.data(), (int)ph.Values.size(), 0, ph.OverlayText.c_str(), ph.ScaleMin, ph.ScaleMax, {ph.GraphSize.x, ph.GraphSize.y});
                if (ImGui::IsItemActive()) itemHandled = true;
            }

            if (entity.HasComponent<TabBarControl>())
            {
                auto& tb = entity.GetComponent<TabBarControl>();
                ImGuiTabBarFlags flags = (tb.Reorderable ? ImGuiTabBarFlags_Reorderable : 0) | (tb.AutoSelectNewTabs ? ImGuiTabBarFlags_AutoSelectNewTabs : 0);
                if (ImGui::BeginTabBar(tb.Label.c_str(), flags)) ImGui::EndTabBar();
            }

            if (entity.HasComponent<TabItemControl>())
            {
                auto& ti = entity.GetComponent<TabItemControl>();
                if (ImGui::BeginTabItem(ti.Label.c_str(), &ti.IsOpen)) { ti.Selected = true; ImGui::EndTabItem(); }
                else ti.Selected = false;
            }

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
                    control.Transform.OffsetMin.x += delta.x;
                    control.Transform.OffsetMax.x += delta.x;
                    control.Transform.OffsetMin.y += delta.y;
                    control.Transform.OffsetMax.y += delta.y;
                }
            }

            ImGui::PopID();
            ImGui::EndGroup();
        }
    }
}

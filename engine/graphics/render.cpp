#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/scene/entity.h"
#include "imgui.h"
#include "rlgl.h"
#include <algorithm>
#include <vector>

namespace CHEngine
{
    RendererState Render::s_State;

    void Render::Init()
    {
        CH_CORE_INFO("Initializing Render System...");
        
        // Initialize underlying API context if needed
        // For Raylib/OpenGL, we ensure initial state is clean
        s_State = RendererState();
        
        InitSkybox();
        
        CH_CORE_INFO("Render System Initialized.");
    }

    void Render::Shutdown()
    {
        CH_CORE_INFO("Shutting down Render System...");
        // Cleanup resources
    }

    void Render::BeginScene(const Camera3D& camera)
    {
        // Preparation for scene rendering (e.g. updating internal buffers)
        BeginMode3D(camera);
    }

    void Render::EndScene()
    {
        EndMode3D();
    }

    void Render::DrawScene(Scene* scene, const Camera3D& camera, Timestep ts, const DebugRenderFlags* debugFlags)
    {
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        // 1. Environmental setup
        ApplyEnvironment(scene->GetEnvironment());
        
        // 2. Scene rendering flow
        DrawSkybox(scene->GetEnvironment().Skybox, camera);
        
        BeginScene(camera);
        {
            RenderModels(scene, ts);
            
            if (debugFlags)
                RenderDebug(scene, debugFlags);
                
            RenderEditorIcons(scene, camera);
        }
        EndScene();
    }

    void Render::Clear(Color color)
    {
        ClearBackground(color);
    }

    void Render::SetViewport(int x, int y, int width, int height)
    {
        // Raylib/OpenGL Viewport
        rlViewport(x, y, width, height);
    }

    void Render::DrawModel(const std::string& path, const Matrix& transform, 
                         const std::vector<MaterialSlot>& overrides, 
                         int animIndex, int frame)
    {
        auto modelAsset = AssetManager::Get<ModelAsset>(path);
        if (!modelAsset) return;

        // Implementation of model drawing with overrides
        // ... (Transplanted logic from DrawCommand)
        DrawMesh(modelAsset->GetModel().meshes[0], modelAsset->GetModel().materials[0], transform);
    }

    void Render::DrawLine(Vector3 start, Vector3 end, Color color)
    {
        DrawLine3D(start, end, color);
    }

    void Render::DrawGrid(int slices, float spacing)
    {
        DrawGrid(slices, spacing); // Raylib built-in
    }

    void Render::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
    {
        // Implementation transplanted from DrawCommand
        // ...
    }

    void Render::DrawUI(Scene* scene, const ImVec2& refPos, const ImVec2& refSize, bool editMode)
    {
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        ImVec2 referenceSize = refSize;
        if (referenceSize.x <= 0 || referenceSize.y <= 0)
            referenceSize = ImGui::GetIO().DisplaySize;

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
            
            ImVec2 pos = {rect.Min.x - refPos.x, rect.Min.y - refPos.y};
            ImVec2 size = {rect.Size().x, rect.Size().y};

            ImGui::SetCursorPos(pos);
            ImGui::BeginGroup();
            ImGui::PushID((int)entityID);
            
            // Render Panel
            if (entity.HasComponent<PanelControl>())
            {
                auto& pnl = entity.GetComponent<PanelControl>();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{pnl.Style.BackgroundColor.r / 255.0f, pnl.Style.BackgroundColor.g / 255.0f, pnl.Style.BackgroundColor.b / 255.0f, pnl.Style.BackgroundColor.a / 255.0f});
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, pnl.Style.Rounding);
                ImGui::BeginChild("Panel", size, true); 
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }

            // Render Label
            if (entity.HasComponent<LabelControl>())
            {
                auto& lbl = entity.GetComponent<LabelControl>();
                ImVec4 color = {lbl.Style.TextColor.r / 255.0f, lbl.Style.TextColor.g / 255.0f, lbl.Style.TextColor.b / 255.0f, lbl.Style.TextColor.a / 255.0f};
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::PushTextWrapPos(pos.x + size.x);

                ImVec2 textSize = ImGui::CalcTextSize(lbl.Text.c_str(), nullptr, true, size.x);
                float startX = pos.x;
                if (lbl.Style.HorizontalAlignment == TextAlignment::Center) startX += (size.x - textSize.x) * 0.5f;
                else if (lbl.Style.HorizontalAlignment == TextAlignment::Right) startX += (size.x - textSize.x);

                float startY = pos.y;
                if (lbl.Style.VerticalAlignment == TextAlignment::Center) startY += (size.y - textSize.y) * 0.5f;
                else if (lbl.Style.VerticalAlignment == TextAlignment::Right) startY += (size.y - textSize.y);

                ImGui::SetCursorPos({startX, startY});
                ImGui::TextUnformatted(lbl.Text.c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
            }

            // Render Button
            if (entity.HasComponent<ButtonControl>())
            {
                auto& btn = entity.GetComponent<ButtonControl>();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{btn.Style.BackgroundColor.r / 255.0f, btn.Style.BackgroundColor.g / 255.0f, btn.Style.BackgroundColor.b / 255.0f, btn.Style.BackgroundColor.a / 255.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{btn.Style.HoverColor.r / 255.0f, btn.Style.HoverColor.g / 255.0f, btn.Style.HoverColor.b / 255.0f, btn.Style.HoverColor.a / 255.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{btn.Style.PressedColor.r / 255.0f, btn.Style.PressedColor.g / 255.0f, btn.Style.PressedColor.b / 255.0f, btn.Style.PressedColor.a / 255.0f});
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{btn.Text.TextColor.r / 255.0f, btn.Text.TextColor.g / 255.0f, btn.Text.TextColor.b / 255.0f, btn.Text.TextColor.a / 255.0f});
                
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btn.Style.Rounding);
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{
                    btn.Text.HorizontalAlignment == TextAlignment::Left ? 0.0f : (btn.Text.HorizontalAlignment == TextAlignment::Center ? 0.5f : 1.0f),
                    btn.Text.VerticalAlignment == TextAlignment::Left ? 0.0f : (btn.Text.VerticalAlignment == TextAlignment::Center ? 0.5f : 1.0f)
                });
                
                if (ImGui::Button(btn.Label.c_str(), size)) btn.PressedThisFrame = true;

                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(4);
            }

            if (entity.HasComponent<SliderControl>())
            {
                auto& sl = entity.GetComponent<SliderControl>();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{sl.Text.TextColor.r / 255.0f, sl.Text.TextColor.g / 255.0f, sl.Text.TextColor.b / 255.0f, sl.Text.TextColor.a / 255.0f});
                ImGui::SetNextItemWidth(size.x * 0.7f);
                sl.Changed = ImGui::SliderFloat(sl.Label.c_str(), &sl.Value, sl.Min, sl.Max);
                ImGui::PopStyleColor();
            }

            if (entity.HasComponent<CheckboxControl>())
            {
                auto& cb = entity.GetComponent<CheckboxControl>();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{cb.Text.TextColor.r / 255.0f, cb.Text.TextColor.g / 255.0f, cb.Text.TextColor.b / 255.0f, cb.Text.TextColor.a / 255.0f});
                cb.Changed = ImGui::Checkbox(cb.Label.c_str(), &cb.Checked);
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
            ImGui::EndGroup();

            if (editMode) {
                ImGui::SetCursorPos(pos);
                ImGui::InvisibleButton("##SelectionZone", size);
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    cc.Transform.OffsetMin.x += delta.x;
                    cc.Transform.OffsetMax.x += delta.x;
                    cc.Transform.OffsetMin.y += delta.y;
                    cc.Transform.OffsetMax.y += delta.y;
                }
            }
        }
    }

    RendererState& Render::GetState()
    {
        return s_State;
    }

    void Render::SetDirectionalLight(Vector3 direction, Color color)
    {
        s_State.DirLightDirection = direction;
        s_State.DirLightColor = color;
    }

    void Render::SetAmbientLight(float intensity)
    {
        s_State.AmbientIntensity = intensity;
    }

    void Render::ApplyEnvironment(const EnvironmentSettings& settings)
    {
        SetAmbientLight(settings.AmbientIntensity);
        SetDirectionalLight(settings.LightDirection, settings.LightColor);
        // ... apply fog, etc.
    }

    // --- Private Internal Helpers (Merging logic from ScenePipeline) ---

    void Render::RenderModels(Scene* scene, Timestep ts)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<TransformComponent, ModelComponent>();
        
        float targetFPS = 30.0f;
        if (Project::GetActive())
            targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;
        float frameTime = 1.0f / (targetFPS > 0 ? targetFPS : 30.0f);

        for (auto entity : view)
        {
            auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);
            
            // Declarative Animation Posing (Hazel-style: logic moved to render pass)
            int currentFrame = 0;
            if (registry.all_of<AnimationComponent>(entity))
            {
                auto& anim = registry.get<AnimationComponent>(entity);
                if (anim.IsPlaying && model.Asset)
                {
                    int animCount = 0;
                    auto* anims = model.Asset->GetAnimations(&animCount);
                    if (anims && anim.CurrentAnimationIndex < animCount)
                    {
                        // Update frame based on delta time
                        anim.FrameTimeCounter += ts.GetSeconds();
                        while (anim.FrameTimeCounter >= frameTime)
                        {
                            anim.CurrentFrame++;
                            anim.FrameTimeCounter -= frameTime;
                            if (anim.CurrentFrame >= anims[anim.CurrentAnimationIndex].frameCount)
                            {
                                if (anim.IsLooping) anim.CurrentFrame = 0;
                                else {
                                    anim.CurrentFrame = anims[anim.CurrentAnimationIndex].frameCount - 1;
                                    anim.IsPlaying = false;
                                    anim.FrameTimeCounter = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
                currentFrame = anim.CurrentFrame;
                DrawModel(model.ModelPath, transform.GetTransform(), {}, anim.CurrentAnimationIndex, currentFrame);
            }
            else
            {
                DrawModel(model.ModelPath, transform.GetTransform());
            }
        }
    }

    void Render::RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags)
    {
        // Debug visualization (colliders, etc)
    }

    void Render::RenderEditorIcons(Scene* scene, const Camera3D& camera)
    {
        // Billboard-style icons for lights/cameras in editor
    }

    void Render::InitSkybox()
    {
        // Setup internal skybox resources
    }

} // namespace CHEngine

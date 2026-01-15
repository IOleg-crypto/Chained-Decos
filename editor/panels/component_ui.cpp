#include "component_ui.h"
#include "editor_layer.h"
#include "engine/audio/audio_manager.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/script_registry.h"
#include "logic/undo/transform_command.h"
#include <filesystem>
#include <imgui.h>
#include <nfd.h>
#include <raymath.h>

namespace CHEngine
{
namespace ComponentUI
{
static bool DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                            float columnWidth = 100.0f)
{
    bool modified = false;
    ImGui::PushID(label.c_str());
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    float itemWidth = (ImGui::GetContentRegionAvail().x - columnWidth) / 3.0f;
    ImGui::PushItemWidth(itemWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = ImGui::GetFrameHeight();
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
    {
        values.x = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
    {
        values.y = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
    {
        values.z = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
    return modified;
}

template <typename T, typename UIFunction>
static void DrawComponent(const std::string &name, Entity entity, UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    if (entity.HasComponent<T>())
    {
        auto &component = entity.GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
        float lineHeight = ImGui::GetFrameHeight();
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void *)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();

        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
            ImGui::OpenPopup("ComponentSettings");

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
                removeComponent = true;
            ImGui::EndPopup();
        }

        if (open)
        {
            uiFunction(component);
            ImGui::TreePop();
        }

        if (removeComponent)
            entity.RemoveComponent<T>();
    }
}

void DrawTag(Entity entity)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto &tag = entity.GetComponent<TagComponent>().Tag;
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            tag = std::string(buffer);
    }
}

void DrawTransform(Entity entity)
{
    DrawComponent<TransformComponent>(
        "Transform", entity,
        [&](auto &transform)
        {
            static TransformComponent oldTransform;
            if (DrawVec3Control("Translation", transform.Translation))
            {
            }
            Vector3 rotation = transform.Rotation;
            if (DrawVec3Control("Rotation", rotation))
                transform.Rotation = rotation;
            DrawVec3Control("Scale", transform.Scale, 1.0f);

            if (ImGui::IsItemActivated())
                oldTransform = transform;
            if (ImGui::IsItemDeactivatedAfterEdit())
                EditorLayer::GetCommandHistory().PushCommand(
                    std::make_unique<TransformCommand>(entity, oldTransform, transform));
        });
}

void DrawModel(Entity entity)
{
    DrawComponent<ModelComponent>(
        "Model", entity,
        [](auto &model)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, model.ModelPath.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("Model Path", buffer, sizeof(buffer)))
                model.ModelPath = std::string(buffer);

            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Model Files", "obj,glb,gltf,iqm,msh"}};
                if (NFD_OpenDialog(&outPath, filterItem, 1, NULL) == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        model.ModelPath = (!ec) ? relativePath.string() : fullPath.string();
                    }
                    else
                        model.ModelPath = fullPath.string();
                    NFD_FreePath(outPath);
                }
            }
            if (ImGui::Button("Load / Reload"))
                AssetManager::LoadModel(model.ModelPath);
            DrawVec3Control("Scale", model.Scale, 1.0f);
        });
}

void DrawCollider(Entity entity)
{
    DrawComponent<ColliderComponent>(
        "Collider", entity,
        [](auto &collider)
        {
            const char *types[] = {"Box (AABB)", "Mesh (BVH)"};
            int currentType = (int)collider.Type;
            if (ImGui::Combo("Type", &currentType, types, 2))
                collider.Type = (ColliderType)currentType;
            ImGui::Checkbox("Enabled", &collider.bEnabled);
            if (collider.Type == ColliderType::Box)
            {
                ImGui::Checkbox("Auto Calculate", &collider.bAutoCalculate);
                if (!collider.bAutoCalculate)
                {
                    DrawVec3Control("Offset", collider.Offset);
                    DrawVec3Control("Size", collider.Size, 1.0f);
                }
            }
            ImGui::Text("Colliding: %s", collider.IsColliding ? "YES" : "NO");
        });
}

void DrawRigidBody(Entity entity)
{
    DrawComponent<RigidBodyComponent>(
        "Rigid Body", entity,
        [](auto &rigidBody)
        {
            DrawVec3Control("Velocity", rigidBody.Velocity);
            ImGui::Checkbox("Use Gravity", &rigidBody.UseGravity);
            ImGui::Checkbox("Is Kinematic", &rigidBody.IsKinematic);
            ImGui::DragFloat("Mass", &rigidBody.Mass, 0.1f, 0.1f, 100.0f);
            ImGui::Text("Grounded: %s", rigidBody.IsGrounded ? "YES" : "NO");
        });
}

void DrawSpawn(Entity entity)
{
    DrawComponent<SpawnComponent>("Spawn Zone", entity,
                                  [](auto &spawn)
                                  {
                                      ImGui::Checkbox("Active", &spawn.IsActive);
                                      DrawVec3Control("Zone Size", spawn.ZoneSize, 1.0f);
                                  });
}

void DrawPlayer(Entity entity)
{
    DrawComponent<PlayerComponent>("Player", entity,
                                   [](auto &player)
                                   {
                                       ImGui::DragFloat("Speed", &player.MovementSpeed, 0.1f);
                                       ImGui::DragFloat("Look Sense", &player.LookSensitivity,
                                                        0.01f);
                                       ImGui::DragFloat("Yaw", &player.CameraYaw, 1.0f);
                                       ImGui::DragFloat("Pitch", &player.CameraPitch, 1.0f);
                                       ImGui::DragFloat("Dist", &player.CameraDistance, 0.1f);
                                   });
}

void DrawPointLight(Entity entity)
{
    DrawComponent<PointLightComponent>(
        "Point Light", entity,
        [](auto &light)
        {
            float color[4] = {light.LightColor.r / 255.f, light.LightColor.g / 255.f,
                              light.LightColor.b / 255.f, light.LightColor.a / 255.f};
            if (ImGui::ColorEdit4("Color", color))
            {
                light.LightColor = {
                    (unsigned char)(color[0] * 255), (unsigned char)(color[1] * 255),
                    (unsigned char)(color[2] * 255), (unsigned char)(color[3] * 255)};
            }
            ImGui::DragFloat("Radiance", &light.Radiance, 0.1f);
            ImGui::DragFloat("Radius", &light.Radius, 0.1f);
        });
}

void DrawAudio(Entity entity)
{
    DrawComponent<AudioComponent>("Audio", entity,
                                  [](auto &audio)
                                  {
                                      char buffer[256];
                                      memset(buffer, 0, sizeof(buffer));
                                      strncpy(buffer, audio.SoundPath.c_str(), sizeof(buffer) - 1);
                                      if (ImGui::InputText("Sound Path", buffer, sizeof(buffer)))
                                          audio.SoundPath = std::string(buffer);
                                      ImGui::DragFloat("Volume", &audio.Volume, 0.01f, 0.0f, 1.0f);
                                      ImGui::Checkbox("Loop", &audio.Loop);
                                  });
}

void DrawHierarchy(Entity entity)
{
    DrawComponent<HierarchyComponent>("Hierarchy", entity,
                                      [](auto &hierarchy)
                                      {
                                          ImGui::Text("Parent: %d", (uint32_t)hierarchy.Parent);
                                          ImGui::Text("Children: %zu", hierarchy.Children.size());
                                      });
}

void DrawNativeScript(Entity entity)
{
    DrawComponent<NativeScriptComponent>(
        "Native Scripts", entity,
        [&](auto &nsc)
        {
            const auto &registeredScripts = ScriptRegistry::GetScripts();

            // Display existing scripts
            for (size_t i = 0; i < nsc.Scripts.size(); i++)
            {
                auto &script = nsc.Scripts[i];
                ImGui::PushID(i);

                ImGui::Text("Script: %s", script.ScriptName.c_str());
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
                if (ImGui::Button("Remove"))
                {
                    nsc.Scripts.erase(nsc.Scripts.begin() + i);
                    i--;
                }
                ImGui::PopID();
            }

            ImGui::Separator();

            if (ImGui::Button("Add Script"))
                ImGui::OpenPopup("AddNativeScriptPopup");

            if (ImGui::BeginPopup("AddNativeScriptPopup"))
            {
                for (auto const &[name, functions] : registeredScripts)
                {
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        ScriptRegistry::AddScript(name, nsc);
                    }
                }
                ImGui::EndPopup();
            }
        });
}

void DrawAnimation(Entity entity)
{
    DrawComponent<AnimationComponent>(
        "Animation", entity,
        [](auto &animation)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, animation.AnimationPath.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("Path", buffer, sizeof(buffer)))
                animation.AnimationPath = std::string(buffer);
            ImGui::DragInt("Index", &animation.CurrentAnimationIndex, 1, 0, 100);
            ImGui::Checkbox("Loop", &animation.IsLooping);
            ImGui::Checkbox("Play", &animation.IsPlaying);
        });
}

void DrawAddComponentPopup(Entity entity)
{
    if (ImGui::BeginPopup("AddComponent"))
    {
        if (ImGui::MenuItem("Transform") && !entity.HasComponent<TransformComponent>())
            entity.AddComponent<TransformComponent>();
        if (ImGui::MenuItem("Model") && !entity.HasComponent<ModelComponent>())
            entity.AddComponent<ModelComponent>();
        if (ImGui::MenuItem("Collider") && !entity.HasComponent<ColliderComponent>())
            entity.AddComponent<ColliderComponent>();
        if (ImGui::MenuItem("RigidBody") && !entity.HasComponent<RigidBodyComponent>())
            entity.AddComponent<RigidBodyComponent>();
        if (ImGui::MenuItem("Player") && !entity.HasComponent<PlayerComponent>())
            entity.AddComponent<PlayerComponent>();
        if (ImGui::MenuItem("Animation") && !entity.HasComponent<AnimationComponent>())
            entity.AddComponent<AnimationComponent>();
        if (ImGui::MenuItem("Native Script") && !entity.HasComponent<NativeScriptComponent>())
            entity.AddComponent<NativeScriptComponent>();
        ImGui::EndPopup();
    }
}
} // namespace ComponentUI
} // namespace CHEngine

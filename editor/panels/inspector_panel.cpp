#include "inspector_panel.h"
#include "editor_layer.h"
#include "engine/audio/audio_manager.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_serializer.h"
#include "logic/undo/transform_command.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>
#include <raymath.h>

namespace CH
{
static bool DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                            float columnWidth = 100.0f)
{
    bool modified = false;
    ImGuiIO &io = ImGui::GetIO();

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
    {
        values.x = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
    {
        values.y = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
        modified = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
    {
        values.z = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor(3);

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
        float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void *)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();

        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

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

void InspectorPanel::OnImGuiRender(Scene *scene, Entity entity, bool readOnly)
{
    ImGui::Begin("Inspector");
    if (entity)
    {
        ImGui::BeginDisabled(readOnly);
        DrawComponents(entity);
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::Text("Selection: None");
        ImGui::TextDisabled("Select an entity to view its components.");
    }
    ImGui::End();
}

void InspectorPanel::DrawComponents(Entity entity)
{
    DrawTagComponent(entity);

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);

    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    DrawAddComponentPopup(entity);

    ImGui::PopItemWidth();

    DrawTransformComponent(entity);
    DrawModelComponent(entity);
    DrawColliderComponent(entity);
    DrawRigidBodyComponent(entity);
    DrawSpawnComponent(entity);
    DrawPlayerComponent(entity);
    DrawMaterialComponent(entity);
    DrawPointLightComponent(entity);
    DrawAudioComponent(entity);
    DrawHierarchyComponent(entity);
    DrawCSharpScriptComponent(entity);
}

void InspectorPanel::DrawTagComponent(Entity entity)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto &tag = entity.GetComponent<TagComponent>().Tag;

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, tag.c_str());
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tag = std::string(buffer);
        }
    }
}

void InspectorPanel::DrawAddComponentPopup(Entity entity)
{
    if (ImGui::BeginPopup("AddComponent"))
    {
        if (ImGui::MenuItem("Tag"))
        {
            if (!entity.HasComponent<TagComponent>())
                entity.AddComponent<TagComponent>("New Entity");
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Transform"))
        {
            if (!entity.HasComponent<TransformComponent>())
                entity.AddComponent<TransformComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Model"))
        {
            if (!entity.HasComponent<ModelComponent>())
                entity.AddComponent<ModelComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Collider"))
        {
            if (!entity.HasComponent<ColliderComponent>())
                entity.AddComponent<ColliderComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Spawn Zone"))
        {
            if (!entity.HasComponent<SpawnComponent>())
                entity.AddComponent<SpawnComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Material"))
        {
            if (!entity.HasComponent<MaterialComponent>())
                entity.AddComponent<MaterialComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Audio"))
        {
            if (!entity.HasComponent<AudioComponent>())
                entity.AddComponent<AudioComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Hierarchy"))
        {
            if (!entity.HasComponent<HierarchyComponent>())
                entity.AddComponent<HierarchyComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Script"))
        {
            if (!entity.HasComponent<CSharpScriptComponent>())
                entity.AddComponent<CSharpScriptComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("RigidBody"))
        {
            if (!entity.HasComponent<RigidBodyComponent>())
                entity.AddComponent<RigidBodyComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Player"))
        {
            if (!entity.HasComponent<PlayerComponent>())
                entity.AddComponent<PlayerComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Point Light"))
        {
            if (!entity.HasComponent<PointLightComponent>())
                entity.AddComponent<PointLightComponent>();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void InspectorPanel::DrawTransformComponent(Entity entity)
{
    DrawComponent<TransformComponent>(
        "Transform", entity,
        [&](auto &tc)
        {
            static TransformComponent s_OldTransform;
            bool modified = false;

            if (DrawVec3Control("Translation", tc.Translation))
                modified = true;

            Vector3 rotation = tc.Rotation;
            if (DrawVec3Control("Rotation", rotation))
            {
                tc.Rotation = rotation;
                modified = true;
            }

            if (DrawVec3Control("Scale", tc.Scale, 1.0f))
                modified = true;

            if (ImGui::IsItemActivated())
            {
                s_OldTransform = tc;
            }

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                EditorLayer::GetCommandHistory().PushCommand(
                    std::make_unique<TransformCommand>(entity, s_OldTransform, tc));
            }
        });
}

void InspectorPanel::DrawModelComponent(Entity entity)
{
    DrawComponent<ModelComponent>(
        "Model", entity,
        [](auto &mc)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, mc.ModelPath.c_str());
            if (ImGui::InputText("Model Path", buffer, sizeof(buffer)))
            {
                mc.ModelPath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Model Files", "obj,glb,gltf,iqm,msh"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        if (!ec)
                            mc.ModelPath = relativePath.string();
                        else
                            mc.ModelPath = fullPath.string();
                    }
                    else
                    {
                        mc.ModelPath = fullPath.string();
                    }
                    NFD_FreePath(outPath);
                }
            }

            float color[4] = {mc.Tint.r / 255.0f, mc.Tint.g / 255.0f, mc.Tint.b / 255.0f,
                              mc.Tint.a / 255.0f};
            if (ImGui::ColorEdit4("Tint", color))
            {
                mc.Tint.r = (unsigned char)(color[0] * 255.0f);
                mc.Tint.g = (unsigned char)(color[1] * 255.0f);
                mc.Tint.b = (unsigned char)(color[2] * 255.0f);
                mc.Tint.a = (unsigned char)(color[3] * 255.0f);
            }
        });
}

void InspectorPanel::DrawColliderComponent(Entity entity)
{
    DrawComponent<ColliderComponent>(
        "Collider", entity,
        [](auto &collider)
        {
            const char *types[] = {"Box (AABB)", "Mesh (BVH)"};
            int currentType = (int)collider.Type;
            if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
            {
                collider.Type = (ColliderType)currentType;
            }

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
            else if (collider.Type == ColliderType::Mesh)
            {
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, collider.ModelPath.c_str());
                if (ImGui::InputText("Model Path", buffer, sizeof(buffer)))
                {
                    collider.ModelPath = std::string(buffer);
                    collider.BVHRoot = nullptr;
                }
                ImGui::SameLine();
                if (ImGui::Button("...##MeshCol"))
                {
                    nfdchar_t *outPath = NULL;
                    nfdu8filteritem_t filterItem[1] = {{"Model Files", "obj,glb,gltf,iqm,msh"}};
                    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                    if (result == NFD_OKAY)
                    {
                        std::filesystem::path fullPath = outPath;
                        if (Project::GetActive())
                        {
                            std::filesystem::path assetDir = Project::GetAssetDirectory();
                            std::error_code ec;
                            auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                            if (!ec)
                                collider.ModelPath = relativePath.string();
                            else
                                collider.ModelPath = fullPath.string();
                        }
                        else
                        {
                            collider.ModelPath = fullPath.string();
                        }
                        collider.BVHRoot = nullptr;
                        NFD_FreePath(outPath);
                    }
                }

                if (ImGui::Button("Build BVH") ||
                    (!collider.BVHRoot && !collider.ModelPath.empty()))
                {
                    Model model = AssetManager::LoadModel(collider.ModelPath);
                    if (model.meshCount > 0)
                    {
                        collider.BVHRoot = BVHBuilder::Build(model);
                        BoundingBox box = AssetManager::GetModelBoundingBox(collider.ModelPath);
                        collider.Offset = box.min;
                        collider.Size = Vector3Subtract(box.max, box.min);
                    }
                }
                if (collider.BVHRoot)
                    ImGui::Text("BVH State: Built");
                else
                    ImGui::Text("BVH State: Not Built");
            }

            ImGui::Separator();
            ImGui::Text("Colliding: ");
            ImGui::SameLine();
            ImGui::TextColored(collider.IsColliding ? ImVec4{0, 1, 0, 1} : ImVec4{1, 0, 0, 1},
                               collider.IsColliding ? "YES" : "NO");
        });
}

void InspectorPanel::DrawRigidBodyComponent(Entity entity)
{
    DrawComponent<RigidBodyComponent>("Rigid Body", entity,
                                      [](auto &rb)
                                      {
                                          DrawVec3Control("Velocity", rb.Velocity);
                                          ImGui::Checkbox("Use Gravity", &rb.UseGravity);
                                          ImGui::Checkbox("Is Kinematic", &rb.IsKinematic);
                                          ImGui::DragFloat("Mass", &rb.Mass, 0.1f, 0.1f, 100.0f);

                                          ImGui::Text("Grounded: ");
                                          ImGui::SameLine();
                                          ImGui::TextColored(rb.IsGrounded ? ImVec4{0, 1, 0, 1}
                                                                           : ImVec4{1, 0, 0, 1},
                                                             rb.IsGrounded ? "YES" : "NO");
                                      });
}

void InspectorPanel::DrawSpawnComponent(Entity entity)
{
    DrawComponent<SpawnComponent>("Spawn Zone", entity,
                                  [](auto &sc)
                                  {
                                      ImGui::Checkbox("Active", &sc.IsActive);
                                      DrawVec3Control("Zone Size", sc.ZoneSize, 1.0f);
                                  });
}

void InspectorPanel::DrawPlayerComponent(Entity entity)
{
    DrawComponent<PlayerComponent>(
        "Player", entity,
        [](auto &player)
        {
            ImGui::DragFloat("Movement Speed", &player.MovementSpeed, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Jump Force", &player.JumpForce, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Look Sensitivity", &player.LookSensitivity, 0.01f, 0.1f, 5.0f);
            ImGui::DragFloat("Camera Yaw", &player.CameraYaw, 1.0f, -180.0f, 180.0f);
            ImGui::DragFloat("Camera Pitch", &player.CameraPitch, 1.0f, -89.0f, 89.0f);
            ImGui::DragFloat("Camera Distance", &player.CameraDistance, 0.1f, 2.0f, 40.0f);

            ImGui::Separator();
            ImGui::Text("Debug Info:");
            ImGui::Text("Coordinates: %.2f, %.2f, %.2f", player.coordinates.x, player.coordinates.y,
                        player.coordinates.z);
        });
}

void InspectorPanel::DrawMaterialComponent(Entity entity)
{
    DrawComponent<MaterialComponent>(
        "Material", entity,
        [](auto &mc)
        {
            float color[4] = {mc.AlbedoColor.r / 255.0f, mc.AlbedoColor.g / 255.0f,
                              mc.AlbedoColor.b / 255.0f, mc.AlbedoColor.a / 255.0f};
            if (ImGui::ColorEdit4("Albedo Color", color))
            {
                mc.AlbedoColor.r = (unsigned char)(color[0] * 255.0f);
                mc.AlbedoColor.g = (unsigned char)(color[1] * 255.0f);
                mc.AlbedoColor.b = (unsigned char)(color[2] * 255.0f);
                mc.AlbedoColor.a = (unsigned char)(color[3] * 255.0f);
            }

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, mc.AlbedoPath.c_str(), sizeof(buffer));
            if (ImGui::InputText("Albedo Path", buffer, sizeof(buffer)))
            {
                mc.AlbedoPath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("...##Albedo"))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Image Files", "png,jpg,jpeg,bmp,tga"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        if (!ec)
                            mc.AlbedoPath = relativePath.string();
                        else
                            mc.AlbedoPath = fullPath.string();
                    }
                    else
                    {
                        mc.AlbedoPath = fullPath.string();
                    }
                    NFD_FreePath(outPath);
                }
            }
        });
}

void InspectorPanel::DrawPointLightComponent(Entity entity)
{
    DrawComponent<PointLightComponent>(
        "Point Light", entity,
        [](auto &component)
        {
            float color[4] = {component.LightColor.r / 255.0f, component.LightColor.g / 255.0f,
                              component.LightColor.b / 255.0f, component.LightColor.a / 255.0f};

            if (ImGui::ColorEdit4("Light Color", color))
            {
                component.LightColor.r = (unsigned char)(color[0] * 255.0f);
                component.LightColor.g = (unsigned char)(color[1] * 255.0f);
                component.LightColor.b = (unsigned char)(color[2] * 255.0f);
                component.LightColor.a = (unsigned char)(color[3] * 255.0f);
            }

            ImGui::DragFloat("Radiance", &component.Radiance, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.0f, 1000.0f);
            ImGui::DragFloat("Falloff", &component.Falloff, 0.1f, 0.0f, 20.0f);
        });
}

void InspectorPanel::DrawAudioComponent(Entity entity)
{
    DrawComponent<AudioComponent>(
        "Audio", entity,
        [](auto &ac)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, ac.SoundPath.c_str());
            if (ImGui::InputText("Sound Path", buffer, sizeof(buffer)))
            {
                ac.SoundPath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("...##Audio"))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Audio Files", "wav,ogg,mp3,flac"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        if (!ec)
                            ac.SoundPath = relativePath.string();
                        else
                            ac.SoundPath = fullPath.string();
                    }
                    else
                    {
                        ac.SoundPath = fullPath.string();
                    }
                    NFD_FreePath(outPath);
                }
            }

            ImGui::DragFloat("Volume", &ac.Volume, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Pitch", &ac.Pitch, 0.01f, 0.1f, 2.0f);
            ImGui::Checkbox("Loop", &ac.Loop);
            ImGui::Checkbox("Play On Start", &ac.PlayOnStart);

            if (ImGui::Button("Play"))
            {
                std::filesystem::path path = ac.SoundPath;
                if (!path.is_absolute() && Project::GetActive())
                    path = Project::GetAssetDirectory() / path;
                AudioManager::LoadSound(ac.SoundPath, path.string());
                AudioManager::PlaySound(ac.SoundPath, ac.Volume, ac.Pitch);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
                AudioManager::StopSound(ac.SoundPath);
        });
}

void InspectorPanel::DrawHierarchyComponent(Entity entity)
{
    DrawComponent<HierarchyComponent>("Hierarchy", entity,
                                      [&](auto &hc)
                                      {
                                          ImGui::Text("Parent: %d", (uint32_t)hc.Parent);
                                          if (ImGui::Button("Clear Parent"))
                                              hc.Parent = entt::null;

                                          ImGui::Separator();
                                          ImGui::Text("Children: %zu", hc.Children.size());
                                          for (auto child : hc.Children)
                                          {
                                              ImGui::BulletText("Child Entity: %d",
                                                                (uint32_t)child);
                                          }
                                      });
}

void InspectorPanel::DrawCSharpScriptComponent(Entity entity)
{
    DrawComponent<CSharpScriptComponent>(
        "Script", entity,
        [](auto &sc)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, sc.ClassName.c_str());
            if (ImGui::InputText("Class Name", buffer, sizeof(buffer)))
            {
                sc.ClassName = std::string(buffer);
            }

            ImGui::Checkbox("Initialized", &sc.Initialized);
            ImGui::Text("Handle: %p", sc.Handle);
        });
}
} // namespace CH

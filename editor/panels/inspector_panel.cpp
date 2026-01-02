#include "inspector_panel.h"
#include "editor/fa5_compat.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/editor_scene_manager.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"
#include "nfd.h"
#include "scene/core/scene.h"
#include "scene/ecs/components/core/id_component.h"
#include "scene/ecs/components/core/tag_component.h"
#include "scene/ecs/components/physics_data.h"
#include "scene/ecs/components/player_component.h"
#include "scene/ecs/components/render_component.h"
#include "scene/ecs/components/scripting_components.h"
#include "scene/ecs/components/skybox_component.h"
#include "scene/ecs/components/spawn_point_component.h"
#include "scene/ecs/components/transform_component.h"
#include "scene/ecs/components/utility_components.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <raylib.h>

namespace CHEngine
{

InspectorPanel::InspectorPanel(SelectionManager *selection, CommandHistory *history,
                               EditorSceneManager *sceneManager, EditorSceneActions *sceneActions)
    : m_SelectionManager(selection), m_CommandHistory(history), m_SceneManager(sceneManager),
      m_SceneActions(sceneActions)
{
}

static void DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                            float columnWidth = 100.0f)
{
    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
        values.x = resetValue;
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
        values.z = resetValue;
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

void InspectorPanel::OnImGuiRender()
{
    if (!m_IsVisible)
        return;

    ImGui::Begin("Properties");

    if (m_SelectionManager->HasSelection())
    {
        entt::entity entity = m_SelectionManager->GetSelectedEntity();
        DrawEntityComponents(m_SceneActions->GetActiveScene(), entity);
    }
    else
    {
        ImGui::Text("Select an entity to inspect.");
    }

    ImGui::End();
}

void InspectorPanel::DrawEntityComponents(const std::shared_ptr<Scene> &scene, entt::entity entity)
{
    if (!scene)
        return;
    auto &registry = scene->GetRegistry();

    // Tag Component
    if (registry.all_of<TagComponent>(entity))
    {
        auto &tag = registry.get<TagComponent>(entity);
        char buffer[256];
        strncpy(buffer, tag.Tag.c_str(), sizeof(buffer));
        if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
        {
            tag.Tag = std::string(buffer);
        }
    }

    // ID Component
    if (registry.all_of<IDComponent>(entity))
    {
        auto &id = registry.get<IDComponent>(entity);
        ImGui::TextDisabled("UUID: %llu", (unsigned long long)id.ID);
    }

    ImGui::Separator();

    // Transform Component
    if (registry.all_of<TransformComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &transform = registry.get<TransformComponent>(entity);
            DrawVec3Control("Position", transform.position);
            DrawVec3Control("Rotation", transform.rotation);
            DrawVec3Control("Scale", transform.scale, 1.0f);
        }
    }

    // Render Component
    if (registry.all_of<RenderComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &render = registry.get<RenderComponent>(entity);
            char modelBuffer[256];
            strncpy(modelBuffer, render.modelName.c_str(), sizeof(modelBuffer));
            if (ImGui::InputText("Model", modelBuffer, sizeof(modelBuffer)))
            {
                render.modelName = std::string(modelBuffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("...##ModelBrowse"))
            {
                nfdfilteritem_t filterItem[1] = {
                    {"3D Models (IQM, OBJ, GLTF)", "iqm,obj,gltf,glb"}};
                nfdchar_t *outPath = nullptr;
                if (NFD_OpenDialog(&outPath, filterItem, 1, nullptr) == NFD_OKAY)
                {
                    // For now, extract filename or keep relative path
                    render.modelName = std::filesystem::path(outPath).filename().string();
                    NFD_FreePath(outPath);
                }
            }

            ImGui::ColorEdit4("Tint", (float *)&render.tint);
            ImGui::Checkbox("Visible", &render.visible);
            ImGui::DragInt("Layer", &render.renderLayer, 1.0f, 0, 10);
            DrawVec3Control("Offset", render.offset);
        }
    }

    // Physics Data
    if (registry.all_of<PhysicsData>(entity))
    {
        if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &physics = registry.get<PhysicsData>(entity);
            ImGui::DragFloat("Mass", &physics.mass, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Gravity Scale", &physics.gravity, 0.1f, -20.0f, 20.0f);
            ImGui::DragFloat("Friction", &physics.friction, 0.05f, 0.0f, 1.0f);
            ImGui::DragFloat("Bounciness", &physics.bounciness, 0.05f, 0.0f, 1.0f);
            ImGui::Checkbox("Use Gravity", &physics.useGravity);
            ImGui::Checkbox("Is Kinematic", &physics.isKinematic);

            if (ImGui::TreeNode("Constraints"))
            {
                ImGui::Checkbox("Freeze X", &physics.freezePositionX);
                ImGui::Checkbox("Freeze Y", &physics.freezePositionY);
                ImGui::Checkbox("Freeze Z", &physics.freezePositionZ);
                ImGui::Checkbox("Freeze Rotation", &physics.freezeRotation);
                ImGui::TreePop();
            }
        }
    }

    // Collision Component
    if (registry.all_of<CollisionComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &collision = registry.get<CollisionComponent>(entity);
            ImGui::Checkbox("Is Trigger", &collision.isTrigger);
            ImGui::DragInt("Layer Mask", &collision.collisionLayer, 1, 0, 31);
            ImGui::DragInt("Collision Mask", &collision.collisionMask, 1, 0, 0xFFFFFFFF);

            ImGui::Text("Bounds: Min(%.1f, %.1f, %.1f) Max(%.1f, %.1f, %.1f)",
                        collision.bounds.min.x, collision.bounds.min.y, collision.bounds.min.z,
                        collision.bounds.max.x, collision.bounds.max.y, collision.bounds.max.z);
        }
    }

    // Player Component
    if (registry.all_of<PlayerComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Player (Gameplay)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &player = registry.get<PlayerComponent>(entity);
            ImGui::DragFloat("Move Speed", &player.moveSpeed, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Jump Force", &player.jumpForce, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Sensitivity", &player.mouseSensitivity, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat("Camera Distance", &player.cameraDistance, 0.1f, 1.0f, 20.0f);

            DrawVec3Control("Spawn Point", player.spawnPosition);
        }
    }

    // Skybox Component
    if (registry.all_of<SkyboxComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &skybox = registry.get<SkyboxComponent>(entity);
            ImGui::Text("Texture: %s",
                        skybox.TexturePath.empty() ? "None" : skybox.TexturePath.c_str());
            if (ImGui::Button("...##BrowseSkybox"))
            {
                nfdfilteritem_t filterItem[1] = {
                    {"Skybox Texture (HDR, PNG, JPG)", "hdr,png,jpg,jpeg"}};
                nfdchar_t *outPath = nullptr;
                if (NFD_OpenDialog(&outPath, filterItem, 1, nullptr) == NFD_OKAY)
                {
                    skybox.TexturePath = outPath;
                    NFD_FreePath(outPath);
                }
            }
            ImGui::DragFloat("Gamma", &skybox.GammaValue, 0.1f, 1.0f, 5.0f);
        }
    }

    // Spawn Point Component
    if (registry.all_of<SpawnPointComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Spawn Point", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &spawn = registry.get<SpawnPointComponent>(entity);
            ImGui::InputText("Spawn Name", &spawn.Name[0],
                             spawn.Name.capacity()); // Incomplete but works for now
            ImGui::Checkbox("Is Active", &spawn.IsActive);
            DrawVec3Control("Zone Size", spawn.ZoneSize);
        }
    }

    // Script Component
    if (registry.all_of<CSharpScriptComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &script = registry.get<CSharpScriptComponent>(entity);
            ImGui::Text("Class: %s", script.className.c_str());
            if (ImGui::Button("Browse Script..."))
            {
                // NFD logic
            }
            if (ImGui::Button("Remove Script"))
            {
                registry.remove<CSharpScriptComponent>(entity);
            }
        }
    }
    else
    {
        if (ImGui::Button("Add Component..."))
            ImGui::OpenPopup("AddComponentPopup");

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::MenuItem("Script"))
                registry.emplace<CSharpScriptComponent>(entity);
            if (ImGui::MenuItem("Skybox"))
                registry.emplace<SkyboxComponent>(entity);
            if (ImGui::MenuItem("Spawn Point"))
                registry.emplace<SpawnPointComponent>(entity);
            if (ImGui::MenuItem("Renderer"))
                registry.emplace<RenderComponent>(entity);
            if (ImGui::MenuItem("Physics"))
                registry.emplace<PhysicsData>(entity);
            if (ImGui::MenuItem("Collider"))
                registry.emplace<CollisionComponent>(entity);
            if (ImGui::MenuItem("Player"))
                registry.emplace<PlayerComponent>(entity);
            ImGui::EndPopup();
        }
    }
}

} // namespace CHEngine

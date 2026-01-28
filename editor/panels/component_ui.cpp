#include "component_ui.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor/ui/editor_gui.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "imgui.h"
#include "raymath.h"

namespace CHEngine
{
std::unordered_map<entt::id_type, std::function<void(Entity)>> ComponentUI::s_DrawerRegistry;

void ComponentUI::RegisterDrawer(entt::id_type typeId, std::function<void(Entity)> drawer)
{
    s_DrawerRegistry[typeId] = drawer;
}

void ComponentUI::Init()
{
    Register<TransformComponent>(DrawTransform);
    Register<ModelComponent>(DrawModel);
    Register<ColliderComponent>(DrawCollider);
    Register<RigidBodyComponent>(DrawRigidBody);
    Register<SpawnComponent>(DrawSpawn);
    Register<PlayerComponent>(DrawPlayer);
    Register<PointLightComponent>(DrawPointLight);
    Register<AudioComponent>(DrawAudio);
    Register<HierarchyComponent>(DrawHierarchy);
    Register<NativeScriptComponent>(DrawNativeScript);
    Register<AnimationComponent>(DrawAnimation);
    Register<ControlComponent>(DrawControl);
    Register<PanelControl>(DrawPanelControl);
    Register<LabelControl>(DrawLabelControl);
    Register<ButtonControl>(DrawButtonControl);
    Register<SliderControl>(DrawSliderControl);
    Register<CheckboxControl>(DrawCheckboxControl);
}

void ComponentUI::DrawEntityComponents(Entity entity)
{
    auto &registry = entity.GetScene()->GetRegistry();
    for (auto [id, storage] : registry.storage())
    {
        if (storage.contains(entity))
        {
            if (s_DrawerRegistry.find(id) != s_DrawerRegistry.end())
            {
                ImGui::PushID((int)id);
                s_DrawerRegistry[id](entity);
                ImGui::PopID();
            }
        }
    }
}

void ComponentUI::DrawTag(Entity entity)
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

void ComponentUI::DrawAddComponentPopup(Entity entity)
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
        if (ImGui::MenuItem("Point Light") && !entity.HasComponent<PointLightComponent>())
            entity.AddComponent<PointLightComponent>();
        if (ImGui::MenuItem("Audio") && !entity.HasComponent<AudioComponent>())
            entity.AddComponent<AudioComponent>();

        ImGui::Separator();
        if (ImGui::MenuItem("Native Script") && !entity.HasComponent<NativeScriptComponent>())
            entity.AddComponent<NativeScriptComponent>();

        ImGui::Separator();
        if (ImGui::BeginMenu("UI Controls"))
        {
            if (ImGui::MenuItem("Control Component") && !entity.HasComponent<ControlComponent>())
                entity.AddComponent<ControlComponent>();
            if (ImGui::MenuItem("Button") && !entity.HasComponent<ButtonControl>())
                entity.AddComponent<ButtonControl>();
            if (ImGui::MenuItem("Label") && !entity.HasComponent<LabelControl>())
                entity.AddComponent<LabelControl>();
            if (ImGui::MenuItem("Panel") && !entity.HasComponent<PanelControl>())
                entity.AddComponent<PanelControl>();
            if (ImGui::MenuItem("Slider") && !entity.HasComponent<SliderControl>())
                entity.AddComponent<SliderControl>();
            if (ImGui::MenuItem("Checkbox") && !entity.HasComponent<CheckboxControl>())
                entity.AddComponent<CheckboxControl>();
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

void ComponentUI::DrawTransform(Entity entity)
{
    DrawComponent<TransformComponent>(
        "Transform", entity,
        [](auto &component)
        {
            EditorUI::GUI::BeginProperties();
            EditorUI::GUI::DrawVec3Control("Translation", component.Translation);
            Vector3 rot = Vector3Scale(component.Rotation, RAD2DEG);
            if (EditorUI::GUI::DrawVec3Control("Rotation", rot))
                component.Rotation = Vector3Scale(rot, DEG2RAD);
            EditorUI::GUI::DrawVec3Control("Scale", component.Scale, 1.0f);
            EditorUI::GUI::EndProperties();
        });
}

void ComponentUI::DrawModel(Entity entity)
{
    DrawComponent<ModelComponent>("Model", entity,
                                  [](auto &component)
                                  {
                                      EditorUI::GUI::BeginProperties();
                                      EditorUI::GUI::Property("Path", component.ModelPath);
                                      EditorUI::GUI::EndProperties();
                                  });
}

void ComponentUI::DrawMaterial(Entity entity, int hitMeshIndex)
{
}

void ComponentUI::DrawCollider(Entity entity)
{
    DrawComponent<ColliderComponent>(
        "Collider", entity,
        [](auto &component)
        {
            EditorUI::GUI::BeginProperties();
            EditorUI::GUI::Property("Enabled", component.Enabled);
            EditorUI::GUI::DrawVec3Control("Offset", component.Offset);
            EditorUI::GUI::DrawVec3Control("Size", component.Size, 1.0f);
            EditorUI::GUI::Property("Auto Calculate", component.AutoCalculate);
            EditorUI::GUI::EndProperties();
        });
}

void ComponentUI::DrawRigidBody(Entity entity)
{
    DrawComponent<RigidBodyComponent>("RigidBody", entity,
                                      [](auto &component)
                                      {
                                          EditorUI::GUI::BeginProperties();
                                          EditorUI::GUI::Property("Mass", component.Mass);
                                          EditorUI::GUI::Property("Gravity", component.UseGravity);
                                          EditorUI::GUI::Property("Kinematic",
                                                                  component.IsKinematic);
                                          EditorUI::GUI::EndProperties();
                                      });
}

void ComponentUI::DrawSpawn(Entity entity)
{
    DrawComponent<SpawnComponent>(
        "Spawn", entity,
        [](auto &component)
        {
            EditorUI::GUI::BeginProperties();
            EditorUI::GUI::Property("Active", component.IsActive);
            EditorUI::GUI::DrawVec3Control("Zone Size", component.ZoneSize, 1.0f);
            EditorUI::GUI::DrawVec3Control("Spawn Point", component.SpawnPoint);
            EditorUI::GUI::EndProperties();
        });
}

void ComponentUI::DrawPlayer(Entity entity)
{
    DrawComponent<PlayerComponent>("Player", entity,
                                   [](auto &component)
                                   {
                                       EditorUI::GUI::BeginProperties();
                                       EditorUI::GUI::Property("Speed", component.MovementSpeed);
                                       EditorUI::GUI::Property("Jump", component.JumpForce);
                                       EditorUI::GUI::Property("Sensitivity",
                                                               component.LookSensitivity);
                                       EditorUI::GUI::EndProperties();
                                   });
}

void ComponentUI::DrawPointLight(Entity entity)
{
    DrawComponent<PointLightComponent>("Point Light", entity,
                                       [](auto &component)
                                       {
                                           EditorUI::GUI::BeginProperties();
                                           EditorUI::GUI::Property("Radiance", component.Radiance);
                                           EditorUI::GUI::Property("Radius", component.Radius);
                                           EditorUI::GUI::Property("Falloff", component.Falloff);
                                           EditorUI::GUI::Property("Color", component.LightColor);
                                           EditorUI::GUI::EndProperties();
                                       });
}

void ComponentUI::DrawAudio(Entity entity)
{
    DrawComponent<AudioComponent>("Audio", entity,
                                  [](auto &component)
                                  {
                                      EditorUI::GUI::BeginProperties();
                                      EditorUI::GUI::Property("Path", component.SoundPath);
                                      EditorUI::GUI::Property("Volume", component.Volume);
                                      EditorUI::GUI::EndProperties();
                                  });
}

void ComponentUI::DrawHierarchy(Entity entity)
{
}

void ComponentUI::DrawNativeScript(Entity entity)
{
    DrawComponent<NativeScriptComponent>("Native Script", entity,
                                         [](auto &component)
                                         {
                                             for (auto &script : component.Scripts)
                                                 ImGui::Text("Script: %s",
                                                             script.ScriptName.c_str());
                                         });
}

void ComponentUI::DrawAnimation(Entity entity)
{
    DrawComponent<AnimationComponent>("Animation", entity,
                                      [](auto &component)
                                      {
                                          EditorUI::GUI::BeginProperties();
                                          EditorUI::GUI::Property("Playing", component.IsPlaying);
                                          EditorUI::GUI::EndProperties();
                                      });
}

void ComponentUI::DrawControl(Entity entity)
{
    DrawComponent<ControlComponent>("Control", entity,
                                    [](auto &component)
                                    {
                                        EditorUI::GUI::BeginProperties();
                                        EditorUI::GUI::Property("Active", component.IsActive);
                                        EditorUI::GUI::EndProperties();
                                    });
}

void ComponentUI::DrawPanelControl(Entity entity)
{
    DrawComponent<PanelControl>("Panel", entity,
                                [](auto &component)
                                {
                                    EditorUI::GUI::BeginProperties();
                                    EditorUI::GUI::Property("Color",
                                                            component.Style.BackgroundColor);
                                    EditorUI::GUI::EndProperties();
                                });
}

void ComponentUI::DrawLabelControl(Entity entity)
{
    DrawComponent<LabelControl>("Label", entity,
                                [](auto &component)
                                {
                                    EditorUI::GUI::BeginProperties();
                                    EditorUI::GUI::Property("Text", component.Text);
                                    EditorUI::GUI::EndProperties();
                                });
}

void ComponentUI::DrawButtonControl(Entity entity)
{
    DrawComponent<ButtonControl>("Button", entity,
                                 [](auto &component)
                                 {
                                     EditorUI::GUI::BeginProperties();
                                     EditorUI::GUI::Property("Label", component.Label);
                                     EditorUI::GUI::EndProperties();
                                 });
}

void DrawSliderControl(SliderControl &component) // Error in signature, should be Entity
{
    // Need to match the DrawComponent pattern
}

void ComponentUI::DrawSliderControl(Entity entity)
{
    DrawComponent<SliderControl>("Slider", entity,
                                 [](auto &component)
                                 {
                                     EditorUI::GUI::BeginProperties();
                                     EditorUI::GUI::Property("Value", component.Value);
                                     EditorUI::GUI::EndProperties();
                                 });
}

void ComponentUI::DrawCheckboxControl(Entity entity)
{
    DrawComponent<CheckboxControl>("Checkbox", entity,
                                   [](auto &component)
                                   {
                                       EditorUI::GUI::BeginProperties();
                                       EditorUI::GUI::Property("Value", component.Checked);
                                       EditorUI::GUI::EndProperties();
                                   });
}

void ComponentUI::DrawTextStyle(TextStyle &style)
{
}
void ComponentUI::DrawUIStyle(UIStyle &style)
{
}

} // namespace CHEngine

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
std::unordered_map<entt::id_type, ComponentUI::ComponentMetadata> ComponentUI::s_ComponentRegistry;

void ComponentUI::RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata)
{
    s_ComponentRegistry[typeId] = metadata;
}

void ComponentUI::Init()
{
    Register<TransformComponent>("Transform", DrawTransform);
    Register<ModelComponent>("Model", DrawModel);
    Register<ColliderComponent>("Collider", DrawCollider);
    Register<RigidBodyComponent>("RigidBody", DrawRigidBody);
    Register<PointLightComponent>("Point Light", DrawPointLight);
    Register<AudioComponent>("Audio", DrawAudio);
    Register<AnimationComponent>("Animation", DrawAnimation);
    Register<NativeScriptComponent>("Native Script", DrawNativeScript);

    // UI Components
    Register<ControlComponent>("Control", DrawControl);
    Register<PanelControl>("UI Panel", DrawPanelControl);
    Register<LabelControl>("UI Label", DrawLabelControl);
    Register<ButtonControl>("UI Button", DrawButtonControl);
    Register<SliderControl>("UI Slider", DrawSliderControl);
    Register<CheckboxControl>("UI Checkbox", DrawCheckboxControl);
}

void ComponentUI::DrawEntityComponents(Entity entity)
{
    auto &registry = entity.GetScene()->GetRegistry();
    for (auto [id, storage] : registry.storage())
    {
        if (storage.contains(entity))
        {
            if (s_ComponentRegistry.find(id) != s_ComponentRegistry.end())
            {
                ImGui::PushID((int)id);
                s_ComponentRegistry[id].Draw(entity);
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
        for (auto &[id, metadata] : s_ComponentRegistry)
        {
            // Skip components the entity already has
            auto &registry = entity.GetScene()->GetRegistry();
            if (registry.storage(id)->contains(entity))
                continue;

            if (ImGui::MenuItem(metadata.Name.c_str()))
            {
                metadata.Add(entity);
                ImGui::CloseCurrentPopup();
            }
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

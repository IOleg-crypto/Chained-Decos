#include "property_editor.h"
#include "editor_gui.h"
#include "engine/scene/components.h"
#include "engine/scene/script_registry.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "nfd.h"
#include "raymath.h"
#include "engine/graphics/asset_manager.h"

namespace CHEngine
{

    std::unordered_map<entt::id_type, PropertyEditor::ComponentMetadata> PropertyEditor::s_ComponentRegistry;

    void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata &metadata)
    {
        s_ComponentRegistry[typeId] = metadata;
    }

    static bool DrawTextStyle(TextStyle &style)
    {
        auto pb = EditorGUI::Begin();
        pb.Float("Font Size", style.FontSize).Color("Text Color", style.TextColor);

        const char *alignments[] = {"Left", "Center", "Right"};
        int hAlign = (int)style.HorizontalAlignment;
        if (EditorGUI::Property("H Align", hAlign, alignments, 3)) style.HorizontalAlignment = (TextAlignment)hAlign;

        int vAlign = (int)style.VerticalAlignment;
        if (EditorGUI::Property("V Align", vAlign, alignments, 3)) style.VerticalAlignment = (TextAlignment)vAlign;

        pb.Float("Letter Spacing", style.LetterSpacing).Float("Line Height", style.LineHeight);
        if (pb.Bool("Shadow", style.Shadow) && style.Shadow)
        {
            pb.Float("Shadow Offset", style.ShadowOffset).Color("Shadow Color", style.ShadowColor);
        }
        return pb.Changed;
    }

    static bool DrawUIStyle(UIStyle &style)
    {
        auto pb = EditorGUI::Begin();
        pb.Color("Background", style.BackgroundColor)
          .Color("Hover", style.HoverColor)
          .Color("Pressed", style.PressedColor)
          .Float("Rounding", style.Rounding)
          .Float("Border", style.BorderSize)
          .Color("Border Color", style.BorderColor)
          .Float("Padding", style.Padding);
        return pb.Changed;
    }

    static bool DrawTransform(TransformComponent& comp)
    {
        return PropertyEditor::DrawReflectedProperties<TransformComponent>(comp);
    }

    static bool DrawModel(ModelComponent& comp)
    {
        bool changed = false;
        if (EditorGUI::Property("Path", comp.ModelPath)) changed = true;
        
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Browse##Model"))
        {
            nfdchar_t *outPath = nullptr;
            nfdfilteritem_t filterItem[1] = {{"3D Models", "glb,gltf,obj,fbx"}};
            nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
            if (result == NFD_OKAY && outPath)
            {
                comp.ModelPath = outPath;
                comp.Asset = nullptr;
                NFD_FreePath(outPath);
                changed = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ROTATE " Reload##Model"))
        {
            AssetManager::Clear<ModelAsset>(comp.ModelPath);
            comp.Asset = nullptr;
            changed = true;
        }

        if (!comp.Materials.empty())
        {
            if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                for (size_t i = 0; i < comp.Materials.size(); i++)
                {
                    auto& slot = comp.Materials[i];
                    std::string label = slot.Name + "##" + std::to_string(i);
                    if (ImGui::TreeNodeEx(label.c_str()))
                    {
                        auto pb = EditorGUI::Begin();
                        pb.Color("Albedo Color", slot.Material.AlbedoColor)
                          .Float("Metalness", slot.Material.Metalness, 0.01f, 0.0f, 1.0f)
                          .Float("Roughness", slot.Material.Roughness, 0.01f, 0.0f, 1.0f)
                          .Bool("Double Sided", slot.Material.DoubleSided)
                          .Bool("Transparent", slot.Material.Transparent);
                        if (slot.Material.Transparent)
                            pb.Float("Alpha", slot.Material.Alpha, 0.01f, 0.0f, 1.0f);
                        
                        if (pb.Changed) changed = true;
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
        return changed;
    }

    static bool DrawLight(PointLightComponent& comp)
    {
        return PropertyEditor::DrawReflectedProperties<PointLightComponent>(comp);
    }



    static bool DrawControlComponentUI(ControlComponent &comp)
    {
        bool changed = false;
        auto &rt = comp.Transform;

        // --- Anchor Presets ---
        ImGui::Text("Presets:"); ImGui::SameLine();
        if (ImGui::Button("Center")) {
            rt.AnchorMin = {0.5f, 0.5f}; rt.AnchorMax = {0.5f, 0.5f};
            rt.OffsetMin = {-50, -50}; rt.OffsetMax = {50, 50};
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stretch")) {
            rt.AnchorMin = {0.0f, 0.0f}; rt.AnchorMax = {1.0f, 1.0f};
            rt.OffsetMin = {0, 0}; rt.OffsetMax = {0, 0};
            changed = true;
        }

        // Use compact Property instead of Vec2Control to reduce X/Y label clutter
        if (EditorGUI::Property("Pivot", rt.Pivot)) changed = true;
        if (EditorGUI::Property("Anchor Min", rt.AnchorMin, 0.01f, 0.0f, 1.0f)) changed = true;
        if (EditorGUI::Property("Anchor Max", rt.AnchorMax, 0.01f, 0.0f, 1.0f)) changed = true;

        bool isPoint = (rt.AnchorMin.x == rt.AnchorMax.x && rt.AnchorMin.y == rt.AnchorMax.y);
        if (isPoint)
        {
            float width = rt.OffsetMax.x - rt.OffsetMin.x;
            float height = rt.OffsetMax.y - rt.OffsetMin.y;
            float posX = rt.OffsetMin.x + width * rt.Pivot.x;
            float posY = rt.OffsetMin.y + height * rt.Pivot.y;
            
            glm::vec2 pos = {posX, posY};
            glm::vec2 size = {width, height};

            if (EditorGUI::Property("Pos", pos)) {
                rt.OffsetMin.x = pos.x - size.x * rt.Pivot.x;
                rt.OffsetMin.y = pos.y - size.y * rt.Pivot.y;
                rt.OffsetMax.x = pos.x + size.x * (1.0f - rt.Pivot.x);
                rt.OffsetMax.y = pos.y + size.y * (1.0f - rt.Pivot.y);
                changed = true;
            }
            if (EditorGUI::Property("Size", size)) {
                rt.OffsetMin.x = pos.x - size.x * rt.Pivot.x;
                rt.OffsetMin.y = pos.y - size.y * rt.Pivot.y;
                rt.OffsetMax.x = pos.x + size.x * (1.0f - rt.Pivot.x);
                rt.OffsetMax.y = pos.y + size.y * (1.0f - rt.Pivot.y);
                changed = true;
            }
        }
        else
        {
            // For stretch, show padding (intuitive positive values)
            float rightPadding = -rt.OffsetMax.x;
            float bottomPadding = -rt.OffsetMax.y;

            if (EditorGUI::Property("Left", rt.OffsetMin.x)) changed = true;
            if (EditorGUI::Property("Top", rt.OffsetMin.y)) changed = true;
            if (EditorGUI::Property("Right", rightPadding)) { rt.OffsetMax.x = -rightPadding; changed = true; }
            if (EditorGUI::Property("Bottom", bottomPadding)) { rt.OffsetMax.y = -bottomPadding; changed = true; }
        }

        if (ImGui::TreeNodeEx("Extra Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            if (EditorGUI::Property("Rotation", rt.Rotation)) changed = true;
            if (EditorGUI::Property("Scale", rt.Scale)) changed = true;
            if (EditorGUI::Property("Z Order", comp.ZOrder)) changed = true;
            if (EditorGUI::Property("Visible", comp.IsActive)) changed = true;
            ImGui::TreePop();
        }
        return changed;
    }

    static bool DrawCollider(ColliderComponent& comp)
    {
        auto pb = EditorGUI::Begin();
        pb.Bool("Enabled", comp.Enabled);
        const char* types[] = { "Box", "Mesh (BVH)" };
        int currentType = (int)comp.Type;
        if (EditorGUI::Property("Type", currentType, types, 2)) comp.Type = (ColliderType)currentType;

        if (comp.Type == ColliderType::Box) {
            pb.Vec3("Offset", comp.Offset);
            pb.Vec3("Size", comp.Size);
            pb.Bool("Auto Calculate", comp.AutoCalculate);
        } else {
            pb.String("Model Path", comp.ModelPath);
            ImGui::TextDisabled("BVH Status: %s", comp.BVHRoot ? "Loaded" : "Not Built");
            if (ImGui::Button("Build/Rebuild BVH")) pb.Changed = true;
        }
        return pb.Changed;
    }


    static bool DrawControlWidget(ControlComponent& comp)
    {
        return DrawControlComponentUI(comp);
    }

    static bool DrawButtonWidget(ButtonControl& comp, Entity e)
    {
        bool changed = EditorGUI::Begin()
            .String("Label", comp.Label)
            .Bool("Interactable", comp.IsInteractable);
        
        if (ImGui::TreeNodeEx("Visual Style", ImGuiTreeNodeFlags_Framed))
        {
            if (DrawUIStyle(comp.Style)) changed = true;
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Text Style", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (DrawTextStyle(comp.Text)) changed = true;
            ImGui::TreePop();
        }

        if (e.HasComponent<ControlComponent>())
        {
            if (ImGui::TreeNodeEx("Layout", ImGuiTreeNodeFlags_Framed))
            {
                if (DrawControlComponentUI(e.GetComponent<ControlComponent>())) changed = true;
                ImGui::TreePop();
            }
        }
        return changed;
    }

    // ... similarly for other widgets if needed. For now I'll just clean up the ones I have.


    void PropertyEditor::Init()
    {
        // --- Core & Rendering ---
        Register<TransformComponent>("Transform");
        Register<ModelComponent>("Model", DrawModel);
        Register<PointLightComponent>("Point Light");
        Register<CameraComponent>("Camera");
        Register<AnimationComponent>("Animation");

        // --- Physics ---
        Register<ColliderComponent>("Collider", DrawCollider);
        Register<RigidBodyComponent>("RigidBody");

        Register<AudioComponent>("Audio");
        Register<SpawnComponent>("Spawn Zone");
        Register<PlayerComponent>("Player");
        Register<SceneTransitionComponent>("Scene Transition");
        Register<BillboardComponent>("Billboard");

        Register<NativeScriptComponent>("Native Script", [](auto &comp) {
            bool changed = false;
            for (size_t i = 0; i < comp.Scripts.size(); ++i) {
                ImGui::TextDisabled("Script: %s", comp.Scripts[i].ScriptName.c_str());
                ImGui::SameLine();
                ImGui::PushID((int)i);
                if (ImGui::Button(ICON_FA_TRASH)) {
                    comp.Scripts.erase(comp.Scripts.begin() + i);
                    changed = true;
                    ImGui::PopID(); break; 
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Script")) ImGui::OpenPopup("AddScriptPopup");
            if (ImGui::BeginPopup("AddScriptPopup")) {
                for (const auto& [name, funcs] : ScriptRegistry::GetScripts()) {
                    if (ImGui::MenuItem(name.c_str())) {
                        ScriptRegistry::AddScript(name, comp); changed = true;
                    }
                }
                ImGui::EndPopup();
            }
            return changed;
        });

        // --- UI Widgets (Classic Look) ---
        Register<ControlComponent>("Rect Transform", DrawControlWidget);
        Register<ButtonControl>("Button Widget", DrawButtonWidget);

        // Hide UI internal components from generic "Add Component" menu
        s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = false;
        
        // Mark Widgets
        s_ComponentRegistry[entt::type_hash<ButtonControl>::value()].IsWidget = true;

        s_ComponentRegistry[entt::type_hash<ButtonControl>::value()].AllowAdd = false;
    }

    void PropertyEditor::DrawEntityProperties(CHEngine::Entity entity)
    {
        auto &registry = entity.GetScene()->GetRegistry();
        bool isUI = entity.HasComponent<ControlComponent>();
        
        bool hasWidget = false;
        for (auto [id, storage] : registry.storage())
        {
            if (storage.contains(entity) && s_ComponentRegistry.contains(id))
            {
                if (s_ComponentRegistry[id].IsWidget)
                {
                    hasWidget = true;
                    break;
                }
            }
        }

        for (auto [id, storage] : registry.storage())
        {
            if (storage.contains(entity))
            {
                if (s_ComponentRegistry.find(id) != s_ComponentRegistry.end())
                {
                    auto &metadata = s_ComponentRegistry[id];
                    if (!metadata.Visible)
                        continue;

                    // Logic to reduce clutter
                    if (isUI && id == entt::type_hash<TransformComponent>::value())
                        continue;
                    
                    if (hasWidget && id == entt::type_hash<ControlComponent>::value())
                        continue;

                    ImGui::PushID((int)id);
                    metadata.Draw(entity);
                    ImGui::PopID();
                }
            }
        }
    }

    void PropertyEditor::DrawTag(CHEngine::Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto &tag = entity.GetComponent<TagComponent>().Tag;
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);

            ImGui::Text("Tag");
            ImGui::SameLine();
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                tag = std::string(buffer);
        }
    }

    void PropertyEditor::DrawMaterial(CHEngine::Entity entity, int hitMeshIndex)
    {
    }

    void PropertyEditor::DrawAddComponentPopup(CHEngine::Entity entity)
    {
        if (ImGui::BeginPopup("AddComponent"))
        {
            for (auto &[id, metadata] : s_ComponentRegistry)
            {
                if (!metadata.AllowAdd)
                    continue;

                auto &registry = entity.GetScene()->GetRegistry();
                auto *storage = registry.storage(id);
                if (storage && storage->contains(entity))
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
} // namespace CHEngine

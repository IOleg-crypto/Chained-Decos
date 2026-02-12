#include "property_editor.h"
#include "editor_gui.h"
#include "engine/scene/components.h"
#include "engine/scene/script_registry.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "nfd.h"
#include "raymath.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/scene/project.h"

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
    void PropertyEditor::Init()
    {
#define REG_HIDDEN(T, name) \
        Register<T>(name, [](auto&, auto) { return false; }); \
        s_ComponentRegistry[entt::type_hash<T>::value()].Visible = false;

        // --- Core & Rendering (Migrated to InspectorPanel) ---
        REG_HIDDEN(TransformComponent, "Transform");
        s_ComponentRegistry[entt::type_hash<TransformComponent>::value()].AllowAdd = false;

        REG_HIDDEN(ModelComponent, "Model");
        REG_HIDDEN(PointLightComponent, "Point Light");
        REG_HIDDEN(SpotLightComponent, "Spot Light");

        Register<AnimationComponent>("Animation", [](auto& component, auto entity) { return false; }); 

        // --- Physics (Migrated to InspectorPanel) ---
        REG_HIDDEN(ColliderComponent, "Collider");
        REG_HIDDEN(RigidBodyComponent, "RigidBody");

        // --- Gameplay (Migrated to InspectorPanel) ---
        REG_HIDDEN(AudioComponent, "Audio");
        REG_HIDDEN(SpawnComponent, "Spawn Zone");
        REG_HIDDEN(PlayerComponent, "Player");
        REG_HIDDEN(SceneTransitionComponent, "Scene Transition");
        REG_HIDDEN(NativeScriptComponent, "Native Script");

        // --- UI Widgets (Migrated to InspectorPanel) ---
        REG_HIDDEN(ControlComponent, "Rect Transform");
        REG_HIDDEN(ButtonControl, "Button Widget");
        REG_HIDDEN(PanelControl, "Panel Widget");
        REG_HIDDEN(LabelControl, "Label Widget");
        REG_HIDDEN(SliderControl, "Slider Widget");
        REG_HIDDEN(CheckboxControl, "Checkbox Widget");
        REG_HIDDEN(InputTextControl, "Input Text Widget");
        REG_HIDDEN(ComboBoxControl, "ComboBox Widget");
        REG_HIDDEN(ProgressBarControl, "ProgressBar Widget");
        REG_HIDDEN(ImageControl, "Image Widget");
        REG_HIDDEN(ImageButtonControl, "Image Button Widget");
        REG_HIDDEN(SeparatorControl, "Separator Widget");
        REG_HIDDEN(RadioButtonControl, "RadioButton Widget");
        REG_HIDDEN(ColorPickerControl, "ColorPicker Widget");
        REG_HIDDEN(DragFloatControl, "DragFloat Widget");
        REG_HIDDEN(DragIntControl, "DragInt Widget");
        REG_HIDDEN(TreeNodeControl, "TreeNode Widget");
        REG_HIDDEN(TabBarControl, "TabBar Widget");
        REG_HIDDEN(TabItemControl, "TabItem Widget");
        REG_HIDDEN(CollapsingHeaderControl, "CollapsingHeader Widget");
        REG_HIDDEN(PlotLinesControl, "PlotLines Widget");
        REG_HIDDEN(PlotHistogramControl, "PlotHistogram Widget");

#undef REG_HIDDEN

        // Helper to setup widgets
        auto setupWidget = [](entt::id_type id) {
            auto& metadata = s_ComponentRegistry[id];
            metadata.IsWidget = true;
            metadata.AllowAdd = true;
        };

        setupWidget(entt::type_hash<ButtonControl>::value());
        setupWidget(entt::type_hash<PanelControl>::value());
        setupWidget(entt::type_hash<LabelControl>::value());
        setupWidget(entt::type_hash<SliderControl>::value());
        setupWidget(entt::type_hash<CheckboxControl>::value());
        setupWidget(entt::type_hash<InputTextControl>::value());
        setupWidget(entt::type_hash<ComboBoxControl>::value());
        setupWidget(entt::type_hash<ProgressBarControl>::value());
        setupWidget(entt::type_hash<ImageControl>::value());
        setupWidget(entt::type_hash<ImageButtonControl>::value());
        setupWidget(entt::type_hash<SeparatorControl>::value());
        setupWidget(entt::type_hash<RadioButtonControl>::value());
        setupWidget(entt::type_hash<ColorPickerControl>::value());
        setupWidget(entt::type_hash<DragFloatControl>::value());
        setupWidget(entt::type_hash<DragIntControl>::value());
        setupWidget(entt::type_hash<TreeNodeControl>::value());
        setupWidget(entt::type_hash<TabBarControl>::value());
        setupWidget(entt::type_hash<TabItemControl>::value());
        setupWidget(entt::type_hash<CollapsingHeaderControl>::value());
        setupWidget(entt::type_hash<PlotLinesControl>::value());
        setupWidget(entt::type_hash<PlotHistogramControl>::value());

        // Allow adding Rect Transform directly too
        s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = true;
    }

    void PropertyEditor::DrawEntityProperties(CHEngine::Entity entity)
    {
        auto &registry = entity.GetRegistry();
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
        if (!entity.HasComponent<ModelComponent>())
            return;

        auto &mc = entity.GetComponent<ModelComponent>();
        if (!mc.Asset)
            return;

        const Model &model = mc.Asset->GetModel();
        
        // Helper to draw a single material instance
        auto DrawMaterialInstance = [](MaterialInstance &mat, int index) {
            std::string header = "Material " + std::to_string(index);
            if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushID(index);
                
                // Albedo
                ImGui::Text("Albedo");
                EditorGUI::Property("Color", mat.AlbedoColor);
                EditorGUI::Property("Texture", mat.AlbedoPath, "Texture Files (*.png *.jpg *.tga *.bmp)\0*.png;*.jpg;*.tga;*.bmp\0");
                EditorGUI::Property("Use Texture", mat.OverrideAlbedo);
                
                ImGui::Separator();
                
                // PBR
                ImGui::Text("PBR Properties");
                EditorGUI::Property("Metalness", mat.Metalness, 0.01f, 0.0f, 1.0f);
                EditorGUI::Property("Roughness", mat.Roughness, 0.01f, 0.0f, 1.0f);
                EditorGUI::Property("Normal Map", mat.NormalMapPath, "Texture Files (*.png *.jpg *.tga *.bmp)\0*.png;*.jpg;*.tga;*.bmp\0");
                
                // Rendering
                ImGui::Separator();
                ImGui::Text("Rendering");
                EditorGUI::Property("Double Sided", mat.DoubleSided);
                
                ImGui::PopID();
            }
        };

        if (hitMeshIndex >= 0 && hitMeshIndex < model.meshCount)
        {
            // Find specific material for this mesh
            // 1. Check for Mesh Index override
            int slotIndex = -1;
            for (int i = 0; i < mc.Materials.size(); i++)
            {
                if (mc.Materials[i].Target == MaterialSlotTarget::MeshIndex && mc.Materials[i].Index == hitMeshIndex)
                {
                    slotIndex = i;
                    break;
                }
            }
            
            // 2. Check for Material Index override
            if (slotIndex == -1 && model.meshMaterial)
            {
                int matIndex = model.meshMaterial[hitMeshIndex];
                for (int i = 0; i < mc.Materials.size(); i++)
                {
                    if (mc.Materials[i].Target == MaterialSlotTarget::MaterialIndex && mc.Materials[i].Index == matIndex)
                    {
                        slotIndex = i;
                        break;
                    }
                }
            }

            if (slotIndex != -1)
            {
                DrawMaterialInstance(mc.Materials[slotIndex].Material, slotIndex);
            }
            else
            {
                ImGui::Text("No Material Slot assigned to this mesh.");
                if (ImGui::Button("Create Override"))
                {
                    // Create new slot for this mesh
                    MaterialSlot newSlot;
                    newSlot.Name = "Mesh Override " + std::to_string(hitMeshIndex);
                    newSlot.Target = MaterialSlotTarget::MeshIndex;
                    newSlot.Index = hitMeshIndex;
                    mc.Materials.push_back(newSlot);
                }
            }
        }
        else
        {
            // Show all materials
            for (int i = 0; i < mc.Materials.size(); i++)
            {
                DrawMaterialInstance(mc.Materials[i].Material, i);
            }
        }
    }

    void PropertyEditor::DrawAddComponentPopup(CHEngine::Entity entity)
    {
        if (ImGui::BeginPopup("AddComponent"))
        {
            bool isUIEntity = entity.HasComponent<ControlComponent>();

            for (auto &[id, metadata] : s_ComponentRegistry)
            {
                if (!metadata.AllowAdd)
                    continue;

                // Filtering: Only show widgets if the entity is a UI entity
                // (or if it's the ControlComponent itself which can be added to any transform)
                if (metadata.IsWidget && !isUIEntity)
                    continue;

                auto &registry = entity.GetRegistry();
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

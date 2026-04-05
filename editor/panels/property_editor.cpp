#include "property_editor.h"
#include "editor/editor_layer.h"
#include "editor_gui.h"
#include "ui_properties.h" // Included here to break circular dependency
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_settings.h"
#include "imgui/IconsFontAwesome6.h"
#include "engine/graphics/api/renderer_api.h"
#include "engine/core/dialogs.h"
#include "panel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <memory>


#include "nfd.h"
#include "scripting/scriptengine.h"
#include <Coral/ManagedObject.hpp>

#include <algorithm>
#include <iterator>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{

std::unordered_map<entt::id_type, PropertyEditor::ComponentMetadata> PropertyEditor::s_ComponentRegistry;

// --- Template Implementations (Moved from Header) ---

template <typename T>
void PropertyEditor::DrawComponentReflection(const std::string& name, const char* icon, Entity entity)
{
    DrawComponentContainer<T>(name, icon, entity, [](T& comp, Entity e) {
        UIProperties ui;
        Properties props(ui);
        comp.Reflect(props);
        return props.HasChanged();
    });
}

template <typename T>
void PropertyEditor::DrawComponentContainer(const std::string& name, const char* icon, Entity entity, std::function<bool(T&, Entity)> drawer)
{
    if (entity.HasComponent<T>())
    {
        DrawComponentInternal(entt::type_hash<T>::value(), name, icon, entity, 
            [&]() {
                auto& component = entity.GetComponent<T>();
                T componentCopy = component;
                if (drawer(componentCopy, entity))
                {
                    entity.GetRegistry().template patch<T>(entity, [&componentCopy](T& comp) { comp = componentCopy; });
                    return true;
                }
                return false;
            },
            [&]() { entity.RemoveComponent<T>(); }
        );
    }
}

template <typename T> 
void PropertyEditor::Register(const std::string& name, const char* icon)
{
    ComponentMetadata metadata;
    metadata.Name = name;
    metadata.Icon = icon;
    metadata.Draw = [name, icon](Entity e) { DrawComponentReflection<T>(name, icon, e); };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<T>()) {
            e.AddComponent<T>();
            return true;
        }
        return false;
    };
    RegisterComponent(entt::type_hash<T>::value(), metadata);
}

template <typename T>
void PropertyEditor::RegisterCustom(const std::string& name, std::function<bool(T&, Entity)> drawer, const char* icon)
{
    ComponentMetadata metadata;
    metadata.Name = name;
    metadata.Icon = icon;
    metadata.Draw = [name, icon, drawer](Entity e) { DrawComponentContainer<T>(name, icon, e, drawer); };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<T>()) {
            e.AddComponent<T>();
            return true;
        }
        return false;
    };
    RegisterComponent(entt::type_hash<T>::value(), metadata);
}

// --- Implementation ---

void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata& metadata)
{
    s_ComponentRegistry[typeId] = metadata;
}

void PropertyEditor::Init()
{
    // --- Core Components ---
    Register<TransformComponent>("Transform", ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT);
    s_ComponentRegistry[entt::type_hash<TransformComponent>::value()].AllowAdd = false;

    Register<TagComponent>("Tag", ICON_FA_TAG);
    Register<CameraComponent>("Camera", ICON_FA_VIDEO);
    Register<LightComponent>("Light", ICON_FA_LIGHTBULB);
    Register<RigidBodyComponent>("RigidBody", ICON_FA_CUBES);
    Register<ColliderComponent>("Collider", ICON_FA_SHIELD);
    Register<ModelComponent>("Model", ICON_FA_CUBE);
    Register<MaterialComponent>("Materials", ICON_FA_DROPLET);
    Register<SpriteComponent>("Sprite", ICON_FA_IMAGE);
    Register<PrimitiveComponent>("Primitive", ICON_FA_SHAPES);
    Register<ShaderComponent>("Shader", ICON_FA_CODE);
    Register<AnimationComponent>("Animation", ICON_FA_FILM);
    Register<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH);
    Register<SpawnComponent>("Spawn Zone", ICON_FA_LOCATION_DOT);
    Register<PlayerComponent>("Player", ICON_FA_USER);
    Register<SceneTransitionComponent>("Scene Transition", ICON_FA_DOOR_OPEN);

    // --- Scripting ---
    RegisterCustom<ManagedScriptComponent>("Scripts", [](auto& component, auto entity) {
        bool changed = false;
        for (auto& script : component.Scripts)
        {
            if (ImGui::TreeNodeEx(script.ClassName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto* scriptType = ScriptEngine::Get().GetScriptClass(script.ClassName);
                if (scriptType)
                {
                    auto fieldInfos = scriptType->GetFields();
                    for (auto& info : fieldInfos)
                    {
                        if (info.GetAccessibility() != Coral::TypeAccessibility::Public) continue;
                        std::string fieldName = (std::string)info.GetName();
                        
                        if (script.Fields.find(fieldName) == script.Fields.end())
                        {
                            ScriptField f;
                            f.Name = fieldName;
                            script.Fields[fieldName] = f;
                        }

                        auto& field = script.Fields[fieldName];
                        CHEngine::UIProperties ui;
                        CHEngine::Properties props(ui);
                        field.Reflect(props);
                        
                        if (props.HasChanged())
                        {
                            changed = true;
                            if (script.Instance)
                            {
                                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                                std::visit([&](auto&& v) { obj->SetFieldValue(fieldName, v); }, field.Value);
                            }
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
        return changed;
    }, ICON_FA_FILE_CODE);
    
    // --- UI Components ---
    Register<ControlComponent>("Rect Transform", ICON_FA_VECTOR_SQUARE);
    Register<NavigationComponent>("UI Navigation", ICON_FA_ARROWS_TO_DOT);
    Register<UIActionComponent>("UI Action", ICON_FA_BOLT);

    // --- UI Widgets ---
    Register<ButtonControl>("Button Widget", ICON_FA_ARROW_POINTER);
    Register<PanelControl>("Panel Widget", ICON_FA_WINDOW_MAXIMIZE);
    Register<LabelControl>("Label Widget", ICON_FA_FONT);
    Register<SliderControl>("Slider Widget", ICON_FA_SLIDERS);
    Register<CheckboxControl>("Checkbox Widget", ICON_FA_SQUARE_CHECK);
    Register<InputTextControl>("Input Text Widget", ICON_FA_PEN_TO_SQUARE);
    Register<ComboBoxControl>("ComboBox Widget", ICON_FA_LIST_UL);
    Register<ProgressBarControl>("ProgressBar Widget", ICON_FA_BARS_PROGRESS);
    Register<ImageControl>("Image Widget", ICON_FA_IMAGE);
    Register<ImageButtonControl>("Image Button Widget", ICON_FA_IMAGE);
    Register<SeparatorControl>("Separator Widget", ICON_FA_MINUS);
    Register<RadioButtonControl>("RadioButton Widget", ICON_FA_CIRCLE_DOT);
    Register<ColorPickerControl>("ColorPicker Widget", ICON_FA_PALETTE);
    Register<DragFloatControl>("DragFloat Widget", ICON_FA_ARROWS_LEFT_RIGHT);
    Register<DragIntControl>("DragInt Widget", ICON_FA_ARROWS_LEFT_RIGHT);
    Register<TabBarControl>("TabBar Widget", ICON_FA_TABLE_COLUMNS);
    Register<TabItemControl>("Tab Item Widget", ICON_FA_FILE);
    Register<CollapsingHeaderControl>("CollapsingHeader Widget", ICON_FA_ANGLE_DOWN);
    Register<VerticalLayoutGroup>("Vertical Layout Group", ICON_FA_LAYER_GROUP);

    // Mark widget metadata
    for (auto& [id, metadata] : s_ComponentRegistry)
    {
        if (metadata.Name.find("Widget") != std::string::npos || metadata.Name.find("Group") != std::string::npos)
            metadata.IsWidget = true;
    }
}

void PropertyEditor::DrawComponentInternal(entt::id_type typeId, const std::string& name, const char* icon, 
                                          Entity entity, std::function<bool()> contentDrawer, 
                                          std::function<void()> remover)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                             ImGuiTreeNodeFlags_FramePadding;

    ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
    float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    
    // Header Background Color
    ImGui::PushStyleColor(ImGuiCol_Header, {0.2f, 0.25f, 0.35f, 0.8f});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.3f, 0.4f, 0.6f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.25f, 0.35f, 0.5f, 1.0f});

    std::string headerName = (icon ? std::string(icon) + " " : "") + name;
    bool open = ImGui::TreeNodeEx((void*)typeId, treeNodeFlags, headerName.c_str());
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Right-aligned settings button
    ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.7f);
    ImGui::PushStyleColor(ImGuiCol_Button, {0,0,0,0});
    if (ImGui::Button(ICON_FA_GEAR, ImVec2{lineHeight, lineHeight}))
    {
        ImGui::OpenPopup("ComponentSettings");
    }
    ImGui::PopStyleColor();

    if (ImGui::BeginPopup("ComponentSettings"))
    {
        if (ImGui::MenuItem("Remove Component"))
            remover();

        ImGui::EndPopup();
    }

    if (open)
    {
        EditorGUI::BeginPropertyGrid();
        contentDrawer();
        EditorGUI::EndPropertyGrid();
        ImGui::TreePop();
        ImGui::Spacing();
    }
}


void PropertyEditor::DrawEntityProperties(CHEngine::Entity entity)
{
    auto& registry = entity.GetRegistry();
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
                auto& metadata = s_ComponentRegistry[id];
                if (!metadata.Visible) continue;

                // Logic to reduce clutter
                if (isUI && id == entt::type_hash<TransformComponent>::value()) continue;
                if (hasWidget && id == entt::type_hash<ControlComponent>::value()) continue;

                ImGui::PushID((int)id);
                metadata.Draw(entity);
                ImGui::PopID();
            }
        }
    }
}

void PropertyEditor::DrawEntityHeader(CHEngine::Entity entity)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto& tag = entity.GetComponent<TagComponent>().Tag;
        
        // Entity Icon and Label
        ImGui::BeginGroup();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextColored({0.4f, 0.6f, 0.9f, 1.0f}, ICON_FA_CUBE " Entity");
        ImGui::PopFont();
        
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tag = std::string(buffer);
        }
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS " Add Component", ImVec2(110, 0)))
        {
            ImGui::OpenPopup("AddComponent");
        }
        
        DrawAddComponentPopup(entity);
        ImGui::EndGroup();
        
        ImGui::Spacing();
    }
}

void PropertyEditor::DrawAddComponentPopup(CHEngine::Entity entity)
{
    if (ImGui::BeginPopup("AddComponent"))
    {
        bool isUIEntity = entity.HasComponent<ControlComponent>();
        auto* scene = entity.GetRegistry().ctx().find<Scene*>();
        bool is3DScene = scene && (*scene)->GetSettings().Mode == BackgroundMode::Environment3D;

        for (auto& [id, metadata] : s_ComponentRegistry)
        {
            if (!metadata.AllowAdd) continue;
            if (metadata.IsWidget && !isUIEntity) continue;
            if (is3DScene && (metadata.IsWidget || id == entt::type_hash<ControlComponent>::value())) continue;

            auto& registry = entity.GetRegistry();
            auto* storage = registry.storage(id);
            if (storage && storage->contains(entity)) continue;

            std::string label = (metadata.Icon ? std::string(metadata.Icon) + " " : "") + metadata.Name;
            if (ImGui::MenuItem(label.c_str()))
            {
                metadata.Add(entity);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}
} // namespace CHEngine

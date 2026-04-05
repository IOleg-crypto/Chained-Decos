#include "property_editor.h"
#include "editor/editor_layer.h"
#include "editor/undo/component_commands.h"
#include "editor/undo/modify_component_command.h"
#include "editor_gui.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_settings.h"
#include "imgui.h"
#include "imgui/IconsFontAwesome6.h"
#include "panel.h"
#include "ui_properties.h" // Included here to break circular dependency
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
    static std::unordered_map<entt::entity, T> s_InitialStates;
    entt::entity e = (entt::entity)entity;

    DrawComponentContainer<T>(name, icon, entity, [&](T& comp, Entity ent) {
        UIProperties ui;
        Properties props(ui);
        comp.Reflect(props);

        if (ui.HasStarted())
        {
            s_InitialStates[e] = entity.GetComponent<T>();
        }

        if (ui.HasFinished())
        {
            if (s_InitialStates.contains(e))
            {
                auto oldState = s_InitialStates[e];
                auto newState = comp;
                EditorLayer::GetCommandHistory().PushCommand(
                    std::make_unique<ModifyComponentCommand<T>>(entity, oldState, newState, "Modify " + name));
                s_InitialStates.erase(e);
            }
        }

        return props.HasChanged();
    });
}

template <typename T>
void PropertyEditor::DrawComponentContainer(const std::string& name, const char* icon, Entity entity,
                                            std::function<bool(T&, Entity)> drawer)
{
    if (entity.HasComponent<T>())
    {
        DrawComponentInternal(
            entt::type_hash<T>::value(), name, icon, entity,
            [&]() {
                auto& component = entity.GetComponent<T>();
                T componentCopy = component;
                if (drawer(componentCopy, entity))
                {
                    // Live preview / immediate update
                    entity.GetRegistry().template patch<T>(entity, [&componentCopy](T& comp) { comp = componentCopy; });
                    return true;
                }
                return false;
            },
            [&]() {
                EditorLayer::GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(entity));
            });
    }
}

template <typename T> void PropertyEditor::Register(const std::string& name, const char* icon)
{
    ComponentMetadata metadata;
    metadata.Name = name;
    metadata.Icon = icon;
    metadata.Draw = [name, icon](Entity e) { DrawComponentReflection<T>(name, icon, e); };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<T>())
        {
            EditorLayer::GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
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
        if (!e.HasComponent<T>())
        {
            EditorLayer::GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
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
    Register<SpawnComponent>("SpawnZone", ICON_FA_LOCATION_DOT);
    Register<PlayerComponent>("Player", ICON_FA_USER);
    Register<SceneTransitionComponent>("SceneTransition", ICON_FA_DOOR_OPEN);

    // --- Scripting ---
    RegisterCustom<ManagedScriptComponent>(
        "Scripts",
        [](auto& component, auto entity) {
            // 1. Pre-sync: Discover fields from C# reflection
            for (auto& script : component.Scripts)
            {
                if (script.ClassName.empty())
                {
                    continue;
                }

                auto* scriptClass = ScriptEngine::Get().GetScriptClass(script.ClassName);
                if (scriptClass)
                {
                    auto fieldInfos = scriptClass->GetFields();
                    for (auto& info : fieldInfos)
                    {
                        if (info.GetAccessibility() != Coral::TypeAccessibility::Public)
                        {
                            continue;
                        }
                        std::string fieldName = (std::string)info.GetName();

                        if (script.Fields.find(fieldName) == script.Fields.end())
                        {
                            ScriptField f;
                            f.Name = fieldName;
                            std::string typeName = (std::string)info.GetType().GetFullName();

                            if (typeName == "System.Single")
                            {
                                f.Type = ScriptFieldType::Float;
                                f.Value = 0.0f;
                            }
                            else if (typeName == "System.Int32")
                            {
                                f.Type = ScriptFieldType::Int;
                                f.Value = 0;
                            }
                            else if (typeName == "System.Boolean")
                            {
                                f.Type = ScriptFieldType::Bool;
                                f.Value = false;
                            }
                            else if (typeName == "System.String")
                            {
                                f.Type = ScriptFieldType::String;
                                f.Value = std::string("");
                            }
                            else if (typeName == "CHEngine.Vector2")
                            {
                                f.Type = ScriptFieldType::Vec2;
                                f.Value = glm::vec2(0.0f);
                            }
                            else if (typeName == "CHEngine.Vector3")
                            {
                                f.Type = ScriptFieldType::Vec3;
                                f.Value = glm::vec3(0.0f);
                            }
                            else if (typeName == "CHEngine.Vector4")
                            {
                                f.Type = ScriptFieldType::Vec4;
                                f.Value = glm::vec4(0.0f);
                            }
                            else if (typeName == "CHEngine.Entity")
                            {
                                f.Type = ScriptFieldType::Entity;
                                f.Value = (uint64_t)0;
                            }

                            if (f.Type != ScriptFieldType::None)
                            {
                                script.Fields[fieldName] = f;
                            }
                        }
                    }
                }
            }

            // 2. Standard reflection UI for persistence and serialization
            UIProperties ui;
            Properties props(ui);
            component.Reflect(props);

            if (props.HasChanged())
            {
                // Sync values to live C# instances if they exist
                for (auto& script : component.Scripts)
                {
                    if (script.Instance)
                    {
                        auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                        for (auto& [name, field] : script.Fields)
                        {
                            std::visit([&](auto&& v) { obj->SetFieldValue(name, v); }, field.Value);
                        }
                    }
                }
            }

            // 3. Script selector for adding new scripts
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Add Script:");
            
            static int selectedScriptIdx = 0;
            std::vector<std::string> availableScripts;
            availableScripts.push_back("");
            
            for (const auto& [name, type] : ScriptEngine::Get().GetScriptClasses())
            {
                availableScripts.push_back(name);
            }

            std::vector<const char*> scriptNames;
            for (const auto& name : availableScripts)
            {
                scriptNames.push_back(name.c_str());
            }

            if (ImGui::Combo("##ScriptSelector", &selectedScriptIdx, scriptNames.data(), (int)scriptNames.size()))
            {
                // Selection changed
            }

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_PLUS " Add", ImVec2(-1, 0)))
            {
                if (selectedScriptIdx > 0 && selectedScriptIdx < (int)availableScripts.size())
                {
                    const std::string& scriptName = availableScripts[selectedScriptIdx];
                    
                    // Check if already attached
                    bool alreadyAttached = false;
                    for (const auto& script : component.Scripts)
                    {
                        if (script.ClassName == scriptName)
                        {
                            alreadyAttached = true;
                            break;
                        }
                    }

                    if (!alreadyAttached)
                    {
                        component.Scripts.push_back(ManagedScriptInstance(scriptName));
                        selectedScriptIdx = 0;
                    }
                }
            }

            return props.HasChanged();
        },
        ICON_FA_FILE_CODE);

    // --- UI Components ---
    Register<ControlComponent>("RectTransform", ICON_FA_VECTOR_SQUARE);
    Register<NavigationComponent>("Navigation", ICON_FA_ARROWS_TO_DOT);
    Register<UIActionComponent>("UIAction", ICON_FA_BOLT);

    // --- UI Widgets ---
    Register<ButtonControl>("Button", ICON_FA_ARROW_POINTER);
    Register<PanelControl>("Panel", ICON_FA_WINDOW_MAXIMIZE);
    Register<LabelControl>("Label", ICON_FA_FONT);
    Register<SliderControl>("Slider", ICON_FA_SLIDERS);
    Register<CheckboxControl>("Checkbox", ICON_FA_SQUARE_CHECK);
    Register<InputTextControl>("InputText", ICON_FA_PEN_TO_SQUARE);
    Register<ComboBoxControl>("ComboBox", ICON_FA_LIST_UL);
    Register<ProgressBarControl>("ProgressBar", ICON_FA_BARS_PROGRESS);
    Register<ImageControl>("Image", ICON_FA_IMAGE);
    Register<ImageButtonControl>("ImageButton", ICON_FA_IMAGE);
    Register<SeparatorControl>("Separator", ICON_FA_MINUS);
    Register<RadioButtonControl>("RadioButton", ICON_FA_CIRCLE_DOT);
    Register<ColorPickerControl>("ColorPicker", ICON_FA_PALETTE);
    Register<DragFloatControl>("DragFloat", ICON_FA_ARROWS_LEFT_RIGHT);
    Register<DragIntControl>("DragInt", ICON_FA_ARROWS_LEFT_RIGHT);
    Register<TabBarControl>("TabBar", ICON_FA_TABLE_COLUMNS);
    Register<TabItemControl>("TabItem", ICON_FA_FILE);
    Register<CollapsingHeaderControl>("CollapsingHeader", ICON_FA_ANGLE_DOWN);
    Register<VerticalLayoutGroup>("VerticalLayoutGroup", ICON_FA_LAYER_GROUP);

    // Mark only real UI widget types as IsWidget (these will be hidden in 3D scenes)
    auto markWidget = [&](entt::id_type id) {
        if (s_ComponentRegistry.contains(id))
            s_ComponentRegistry[id].IsWidget = true;
    };
    markWidget(entt::type_hash<ControlComponent>::value());
    markWidget(entt::type_hash<NavigationComponent>::value());
    markWidget(entt::type_hash<UIActionComponent>::value());
    markWidget(entt::type_hash<ButtonControl>::value());
    markWidget(entt::type_hash<PanelControl>::value());
    markWidget(entt::type_hash<LabelControl>::value());
    markWidget(entt::type_hash<SliderControl>::value());
    markWidget(entt::type_hash<CheckboxControl>::value());
    markWidget(entt::type_hash<InputTextControl>::value());
    markWidget(entt::type_hash<ComboBoxControl>::value());
    markWidget(entt::type_hash<ProgressBarControl>::value());
    markWidget(entt::type_hash<ImageControl>::value());
    markWidget(entt::type_hash<ImageButtonControl>::value());
    markWidget(entt::type_hash<SeparatorControl>::value());
    markWidget(entt::type_hash<RadioButtonControl>::value());
    markWidget(entt::type_hash<ColorPickerControl>::value());
    markWidget(entt::type_hash<DragFloatControl>::value());
    markWidget(entt::type_hash<DragIntControl>::value());
    markWidget(entt::type_hash<TabBarControl>::value());
    markWidget(entt::type_hash<TabItemControl>::value());
    markWidget(entt::type_hash<CollapsingHeaderControl>::value());
    markWidget(entt::type_hash<VerticalLayoutGroup>::value());
    markWidget(entt::type_hash<SpriteComponent>::value());
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
    ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
    if (ImGui::Button(ICON_FA_GEAR, ImVec2{lineHeight, lineHeight}))
    {
        ImGui::OpenPopup("ComponentSettings");
    }
    ImGui::PopStyleColor();

    bool removed = false;
    if (ImGui::BeginPopup("ComponentSettings"))
    {
        if (ImGui::MenuItem("Remove Component"))
        {
            remover();
            removed = true;
        }

        ImGui::EndPopup();
    }

    if (open)
    {
        if (!removed)
        {
            EditorGUI::BeginPropertyGrid();
            contentDrawer();
            EditorGUI::EndPropertyGrid();
        }
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
                if (!metadata.Visible)
                {
                    continue;
                }

                // Logic to reduce clutter
                if (isUI && id == entt::type_hash<TransformComponent>::value())
                {
                    continue;
                }
                if (hasWidget && id == entt::type_hash<ControlComponent>::value())
                {
                    continue;
                }

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
            if (!metadata.AllowAdd)
            {
                continue;
            }
            if (metadata.IsWidget && !isUIEntity)
            {
                continue;
            }
            if (is3DScene && (metadata.IsWidget || id == entt::type_hash<ControlComponent>::value()))
            {
                continue;
            }

            auto& registry = entity.GetRegistry();
            auto* storage = registry.storage(id);
            if (storage && storage->contains(entity))
            {
                continue;
            }

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

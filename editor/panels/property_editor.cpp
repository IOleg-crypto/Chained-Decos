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
            bool changed = false;

            // ── Pre-sync: discover public fields from C# reflection ─────────────────
            for (auto& script : component.Scripts)
            {
                if (script.ClassName.empty()) continue;
                auto* scriptClass = ScriptEngine::Get().GetScriptClass(script.ClassName);
                if (!scriptClass) continue;

                for (auto& info : scriptClass->GetFields())
                {
                    if (info.GetAccessibility() != Coral::TypeAccessibility::Public) continue;
                    std::string fieldName = (std::string)info.GetName();
                    if (script.Fields.contains(fieldName)) continue;

                    ScriptField f;
                    f.Name = fieldName;
                    std::string typeName = (std::string)info.GetType().GetFullName();
                    if      (typeName == "System.Single")  { f.Type = ScriptFieldType::Float;  f.Value = 0.0f; }
                    else if (typeName == "System.Int32")   { f.Type = ScriptFieldType::Int;    f.Value = 0; }
                    else if (typeName == "System.Boolean") { f.Type = ScriptFieldType::Bool;   f.Value = false; }
                    else if (typeName == "System.String")  { f.Type = ScriptFieldType::String; f.Value = std::string(""); }
                    else if (typeName == "CHEngine.Vector2"){ f.Type = ScriptFieldType::Vec2;  f.Value = glm::vec2(0.0f); }
                    else if (typeName == "CHEngine.Vector3"){ f.Type = ScriptFieldType::Vec3;  f.Value = glm::vec3(0.0f); }
                    else if (typeName == "CHEngine.Vector4"){ f.Type = ScriptFieldType::Vec4;  f.Value = glm::vec4(0.0f); }
                    else if (typeName == "CHEngine.Entity") { f.Type = ScriptFieldType::Entity; f.Value = (uint64_t)0; }
                    if (f.Type != ScriptFieldType::None) script.Fields[fieldName] = f;
                }
            }

            // ── Draw one card per attached script ───────────────────────────────────
            int toRemove = -1;
            for (int si = 0; si < (int)component.Scripts.size(); ++si)
            {
                auto& script = component.Scripts[si];

                // Extract short class name for the header
                std::string shortName = script.ClassName;
                auto dot = shortName.rfind('.');
                if (dot != std::string::npos) shortName = shortName.substr(dot + 1);

                ImGui::PushID(si);

                // Card background
                ImGui::PushStyleColor(ImGuiCol_Header,        {0.15f, 0.20f, 0.30f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.20f, 0.28f, 0.42f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,  {0.25f, 0.35f, 0.52f, 1.0f});

                bool open = ImGui::TreeNodeEx("##script", ImGuiTreeNodeFlags_DefaultOpen |
                                              ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth |
                                              ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding,
                                              "%s  %s", ICON_FA_FILE_CODE, shortName.c_str());
                ImGui::PopStyleColor(3);

                // Delete button aligned to the right
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.6f, 0.15f, 0.15f, 1.0f});
                if (ImGui::SmallButton(ICON_FA_TRASH))
                {
                    toRemove = si;
                    changed = true;
                }
                ImGui::PopStyleColor(2);

                if (open)
                {
                    // Full class name (muted)
                    ImGui::TextDisabled("  %s", script.ClassName.c_str());
                    ImGui::Spacing();

                    if (script.Fields.empty())
                    {
                        ImGui::TextDisabled("  (no public fields)");
                    }

                    // ── Per-field controls ──────────────────────────────────────────
                    for (auto& [fieldName, field] : script.Fields)
                    {
                        ImGui::PushID(fieldName.c_str());

                        // Label column
                        float labelW = ImGui::GetContentRegionAvail().x * 0.40f;
                        ImGui::Text("%s", fieldName.c_str());
                        ImGui::SameLine(labelW);
                        ImGui::SetNextItemWidth(-1.0f);

                        bool fieldChanged = false;
                        switch (field.Type)
                        {
                        case ScriptFieldType::Float:
                        {
                            float v = std::get<float>(field.Value);
                            if (ImGui::DragFloat("##v", &v, 0.1f)) { field.Value = v; fieldChanged = true; }
                            break;
                        }
                        case ScriptFieldType::Int:
                        {
                            int v = std::get<int>(field.Value);
                            if (ImGui::DragInt("##v", &v)) { field.Value = v; fieldChanged = true; }
                            break;
                        }
                        case ScriptFieldType::Bool:
                        {
                            bool v = std::get<bool>(field.Value);
                            if (ImGui::Checkbox("##v", &v)) { field.Value = v; fieldChanged = true; }
                            break;
                        }
                        case ScriptFieldType::String:
                        {
                            auto& v = std::get<std::string>(field.Value);
                            char buf[256] = {};
                            strncpy(buf, v.c_str(), 255);
                            if (ImGui::InputText("##v", buf, sizeof(buf))) { field.Value = std::string(buf); fieldChanged = true; }
                            break;
                        }
                        case ScriptFieldType::Vec2:
                        {
                            glm::vec2& v = std::get<glm::vec2>(field.Value);
                            if (ImGui::DragFloat2("##v", &v.x, 0.1f)) fieldChanged = true;
                            break;
                        }
                        case ScriptFieldType::Vec3:
                        {
                            glm::vec3& v = std::get<glm::vec3>(field.Value);
                            if (ImGui::DragFloat3("##v", &v.x, 0.1f)) fieldChanged = true;
                            break;
                        }
                        case ScriptFieldType::Vec4:
                        case ScriptFieldType::Color:
                        {
                            glm::vec4& v = std::get<glm::vec4>(field.Value);
                            if (ImGui::ColorEdit4("##v", &v.x)) fieldChanged = true;
                            break;
                        }
                        case ScriptFieldType::Entity:
                        {
                            uint64_t v = std::get<uint64_t>(field.Value);
                            ImGui::TextDisabled("Entity: %llu", (unsigned long long)v);
                            break;
                        }
                        default:
                            ImGui::TextDisabled("(unsupported)");
                            break;
                        }

                        if (fieldChanged)
                        {
                            changed = true;
                            // Live-sync to running C# instance
                            if (script.Instance)
                            {
                                auto* obj = static_cast<Coral::ManagedObject*>(script.Instance);
                                std::visit([&](auto&& v) { obj->SetFieldValue(fieldName, v); }, field.Value);
                            }
                        }

                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                    ImGui::Spacing();
                }

                ImGui::PopID();
            }

            if (toRemove >= 0)
            {
                component.Scripts.erase(component.Scripts.begin() + toRemove);
            }

            // ── Add Script ──────────────────────────────────────────────────────────
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored({0.5f, 0.7f, 1.0f, 1.0f}, ICON_FA_PLUS " Add Script");

            static int s_SelectedScript = 0;
            static std::string s_Filter;

            std::vector<std::string> allScripts;
            allScripts.push_back("-- Select script --");
            for (const auto& [name, type] : ScriptEngine::Get().GetScriptClasses())
                allScripts.push_back(name);

            std::vector<const char*> cnames;
            for (const auto& n : allScripts) cnames.push_back(n.c_str());

            ImGui::SetNextItemWidth(-80.0f);
            ImGui::Combo("##AddScriptCombo", &s_SelectedScript, cnames.data(), (int)cnames.size());
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_PLUS " Add", ImVec2(-1, 0)) && s_SelectedScript > 0
                && s_SelectedScript < (int)allScripts.size())
            {
                const std::string& chosen = allScripts[s_SelectedScript];
                bool already = false;
                for (const auto& s : component.Scripts)
                    if (s.ClassName == chosen) { already = true; break; }
                if (!already)
                {
                    component.Scripts.emplace_back(chosen);
                    changed = true;
                }
                s_SelectedScript = 0;
            }

            return changed;
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

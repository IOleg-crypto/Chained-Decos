#include "property_editor.h"
#include "IconsFontAwesome6.h"
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
    RegisterCustom<ManagedScriptComponent>("Scripts", [](ManagedScriptComponent& comp, Entity entity) {
        bool changed = false;
        
        for (int i = 0; i < (int)comp.Scripts.size(); i++)
        {
            auto& script = comp.Scripts[i];
            ImGui::PushID(i);
            
            // We are already inside a PropertyGrid table (2 columns).
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | 
                                      ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
                                      ImGuiTreeNodeFlags_SpanAllColumns;
            
            // Extract short class name (after last dot)
            std::string fullClassName = script.ClassName;
            size_t lastDot = fullClassName.find_last_of('.');
            std::string shortName = (lastDot == std::string::npos) ? fullClassName : fullClassName.substr(lastDot + 1);
            std::string label = shortName.empty() ? "-- Empty Script --" : shortName;
            
            float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
            bool open = ImGui::TreeNodeEx((void*)(uintptr_t)i, flags, "%s %s", ICON_FA_FILE_CODE, label.c_str());
            
            // Tooltip with full name
            if (ImGui::IsItemHovered() && !fullClassName.empty())
                ImGui::SetTooltip("%s", fullClassName.c_str());

            // Delete button in the header row (right aligned in column 1)
            ImGui::TableSetColumnIndex(1);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - lineHeight - 5.0f);
            if (ImGui::Button(ICON_FA_TRASH, ImVec2{lineHeight, lineHeight}))
            {
                comp.Scripts.erase(comp.Scripts.begin() + i);
                changed = true;
                if (open) ImGui::TreePop();
                ImGui::PopID();
                break; 
            }

            if (open)
            {
                UIProperties ui;
                // Manually draw fields from the map, skipping redundancy
                for (auto& [fieldName, field] : script.Fields)
                {
                    std::visit([&](auto&& val) {
                        if (ui.Property(fieldName.c_str(), val))
                            changed = true;
                    }, field.Value);
                }

                ImGui::TreePop();
            }
            
            ImGui::PopID();
            ImGui::Spacing();
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(1);
        if (EditorGUI::ActionButton(ICON_FA_PLUS, "Add Script"))
        {
            ImGui::OpenPopup("AddScriptPopup");
        }

        if (ImGui::BeginPopup("AddScriptPopup"))
        {
            for (const auto& [className, type] : ScriptEngine::Get().GetScriptClasses())
            {
                // Extract short name for menu
                size_t lastDot = className.find_last_of('.');
                std::string shortName = (lastDot == std::string::npos) ? className : className.substr(lastDot + 1);

                if (ImGui::MenuItem(shortName.c_str()))
                {
                    comp.Scripts.emplace_back(className);
                    changed = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", className.c_str());
            }
            ImGui::EndPopup();
        }

        return changed;
    }, ICON_FA_FILE_CODE);

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
        {
            s_ComponentRegistry[id].IsWidget = true;
        }
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

    // 1. Check for widgets more efficiently
    bool hasWidget = false;
    for (auto [id, storage] : registry.storage())
    {
        if (storage.contains(entity) && s_ComponentRegistry.contains(id) && s_ComponentRegistry[id].IsWidget)
        {
            hasWidget = true;
            break;
        }
    }

    // 2. Draw components efficiently
    for (auto [id, storage] : registry.storage())
    {
        if (storage.contains(entity) && s_ComponentRegistry.contains(id))
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

            ImGui::PushID((int)id);
            metadata.Draw(entity);
            ImGui::PopID();
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

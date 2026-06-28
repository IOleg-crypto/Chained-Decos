#include "property_editor.h"
#include "engine/reflection/reflection_rfl.h"
#include "engine/scene/component_registry.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "editor/editor_layer.h"
#include "editor/undo/component_commands.h"
#include "editor/undo/modify_component_command.h"
#include "engine/core/service_locator.h"
#include "editor_gui.h"

#include "engine/physics/physics.h"
#include "engine/scene/scene_settings.h"
#include "imgui.h"
#include "ui_properties.h" // Included here to break circular dependency
#include <memory>
#include "scripting/scriptengine.h"
#include <Coral/ManagedObject.hpp>

#include "engine/app/application.h"
#include <yaml-cpp/yaml.h>
// Component Registry handles all dynamic component UI
#include "engine/scene/component_registry.h"

namespace Chained
{

EditorLayer* PropertyEditor::s_EditorLayer = nullptr;

// --- Template Implementations (Moved from Header) ---

template <typename T>
void PropertyEditor::DrawComponentReflection(const std::string& name, const char* icon, Entity entity)
{
    static std::unordered_map<entt::entity, T> s_InitialStates;
    entt::entity e = (entt::entity)entity;

    DrawComponentContainer<T>(name, icon, entity, [&](T& comp, Entity ent) {
        UIProperties ui;
        Properties props(ui);
        
        if constexpr (is_rfl_component<T>::value)
            ReflectFromRfl(comp, props);
        else
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
                if (s_EditorLayer)
                {
                    s_EditorLayer->GetCommandHistory().PushCommand(
                        std::make_unique<ModifyComponentCommand<T>>(entity, oldState, newState, "Modify " + name));
                }
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
                    if (s_EditorLayer)
                    {
                        s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(entity));
                    }
            });
    }
}

void PropertyEditor::DrawGenericReflection(const ComponentMetadata& metadata, Entity entity)
{
    // Use a stable hash of the component name as the tree node ID
    // to avoid ImGui ID collisions when multiple generic components are rendered
    entt::id_type stableId = static_cast<entt::id_type>(std::hash<std::string>{}(metadata.Name));
    
    DrawComponentInternal(
        stableId, metadata.Name, metadata.Icon, entity,
        [&]() {
            UIProperties ui;
            metadata.ReflectInternal(entity, &ui, (int)ReflectionMode::UI);
            return ui.HasChanged();
        },
        [&]() {
            if (metadata.Remove) metadata.Remove(entity);
        }
    );
}

template <typename T> void PropertyEditor::Register(const std::string& name, const char* icon)
{
    auto typeId = entt::type_hash<T>::value();
    if (!ComponentRegistry::Exists(typeId))
    {
        ComponentMetadata metadata;
        metadata.Name = name;
        metadata.Icon = icon;
        metadata.Category = "Engine";
        metadata.DrawUI = [name, icon](Entity e) { DrawComponentReflection<T>(name, icon, e); };
        metadata.Add = [](Entity e) {
            if (!e.HasComponent<T>())
            {
                if (s_EditorLayer)
                {
                    s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
                }
            }
        };
        metadata.Remove = [](Entity e) {
            if (s_EditorLayer)
            {
                s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(e));
            }
        };
        metadata.SerializationKey = name + "Component"; // Default key convention
        ComponentRegistry::Register(typeId, metadata);
    }
    else
    {
        auto& metadata = ComponentRegistry::GetMetadataMutable(typeId);
        metadata.Name = name;
        metadata.Icon = icon;
        metadata.DrawUI = [name, icon](Entity e) { DrawComponentReflection<T>(name, icon, e); };
        metadata.Add = [](Entity e) {
            if (!e.HasComponent<T>())
            {
                if (s_EditorLayer)
                    s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
            }
        };
        metadata.Remove = [](Entity e) {
            if (s_EditorLayer)
                s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(e));
        };
    }
}

template <typename T>
void PropertyEditor::RegisterCustom(const std::string& name, std::function<bool(T&, Entity)> drawer, const char* icon)
{
    auto typeId = entt::type_hash<T>::value();
    if (!ComponentRegistry::Exists(typeId))
    {
        ComponentMetadata metadata;
        metadata.Name = name;
        metadata.Icon = icon;
        metadata.Category = "Engine";
        metadata.DrawUI = [name, icon, drawer](Entity e) { DrawComponentContainer<T>(name, icon, e, drawer); };
        metadata.Add = [](Entity e) {
            if (!e.HasComponent<T>())
            {
                if (s_EditorLayer)
                {
                    s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
                }
            }
        };
        metadata.Remove = [](Entity e) {
             if (s_EditorLayer)
             {
                 s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(e));
             }
        };
        ComponentRegistry::Register(typeId, metadata);
    }
    else
    {
        auto& metadata = ComponentRegistry::GetMetadataMutable(typeId);
        metadata.Name = name;
        metadata.Icon = icon;
        metadata.DrawUI = [name, icon, drawer](Entity e) { DrawComponentContainer<T>(name, icon, e, drawer); };
        metadata.Add = [](Entity e) {
            if (!e.HasComponent<T>())
            {
                if (s_EditorLayer)
                {
                    s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
                }
            }
        };
        metadata.Remove = [](Entity e) {
             if (s_EditorLayer)
             {
                 s_EditorLayer->GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(e));
             }
        };
    }
}

// --- Implementation ---

void PropertyEditor::Init()
{
    s_EditorLayer = &EditorLayer::Get();

    // --- Core Components ---
    Register<TransformComponent>("Transform", ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT);
    ComponentRegistry::SetAllowAdd(entt::type_hash<TransformComponent>::value(), false);

    Register<TagComponent>("Tag", ICON_FA_TAG);
    Register<CameraComponent>("Camera", ICON_FA_VIDEO);
    Register<LightComponent>("Light", ICON_FA_LIGHTBULB);
    Register<RigidBodyComponent>("RigidBody", ICON_FA_CUBES);
    Register<ColliderComponent>("Collider", ICON_FA_SHIELD);
    Register<ModelComponent>("Model", ICON_FA_CUBE);
    Register<SpriteComponent>("Sprite", ICON_FA_IMAGE);
    Register<PrimitiveComponent>("Primitive", ICON_FA_SHAPES);
    Register<ShaderComponent>("Shader", ICON_FA_CODE);
    Register<AnimationComponent>("Animation", ICON_FA_FILM);
    Register<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH);

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
            for (const auto& [className, type] : ServiceLocator::Get<ScriptEngine>()->GetRegistry().GetScriptClasses())
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
    Register<UIControlComponent>("Widget", ICON_FA_SHAPES);

    // Mark only real UI widget types as IsWidget (these will be hidden in 3D scenes)
    auto markWidget = [&](entt::id_type id) {
        if (ComponentRegistry::Exists(id))
        {
            auto metadata = ComponentRegistry::GetMetadata(id);
            metadata.IsWidget = true;
            ComponentRegistry::Register(id, metadata);
        }
    };
    markWidget(entt::type_hash<ControlComponent>::value());
    markWidget(entt::type_hash<NavigationComponent>::value());
    markWidget(entt::type_hash<UIActionComponent>::value());
    markWidget(entt::type_hash<UIControlComponent>::value());
    markWidget(entt::type_hash<SpriteComponent>::value());
}

void PropertyEditor::DrawComponentInternal(::entt::id_type typeId, const std::string& name, const char* icon,
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

void PropertyEditor::DrawEntityProperties(Chained::Entity entity)
{
    auto& registry = entity.GetRegistry();
    bool isUI = entity.HasComponent<ControlComponent>();

    auto& compRegistry = ComponentRegistry::GetRegistry();

    // 2. Draw components efficiently
    for (auto [id, storage] : registry.storage())
    {
        if (storage.contains(entity) && compRegistry.contains(id))
        {
            auto& metadata = compRegistry.at(id);
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
            if (metadata.DrawUI)
                metadata.DrawUI(entity);
            else if (metadata.IsReflective && metadata.ReflectInternal)
                DrawGenericReflection(metadata, entity);
            ImGui::PopID();
        }
    }
}

void PropertyEditor::DrawEntityHeader(Chained::Entity entity)
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

void PropertyEditor::DrawAddComponentPopup(Chained::Entity entity)
{
    if (ImGui::BeginPopup("AddComponent"))
    {
        bool isUIEntity = entity.HasComponent<ControlComponent>();
        auto* scene = entity.GetRegistry().ctx().find<Scene*>();
        bool is3DScene = scene && (*scene)->GetSettings().Mode == BackgroundMode::Environment3D;

        // Group components by category
        std::map<std::string, std::vector<const ComponentMetadata*>> categorized;

        for (auto& [id, metadata] : ComponentRegistry::GetRegistry())
        {
            if (!metadata.AllowAdd) continue;
            if (metadata.IsWidget && !isUIEntity) continue;
            if (is3DScene && (metadata.IsWidget || id == entt::type_hash<ControlComponent>::value())) continue;

            auto& registry = entity.GetRegistry();
            auto* storage = registry.storage(id);
            if (storage && storage->contains(entity)) continue;

            categorized[metadata.Category].push_back(&metadata);
        }

        // Render categorized menus
        for (auto& [category, components] : categorized)
        {
            if (ImGui::BeginMenu(category.c_str()))
            {
                for (const auto* metadata : components)
                {
                    std::string label = (metadata->Icon ? std::string(metadata->Icon) + " " : "") + metadata->Name;
                    if (ImGui::MenuItem(label.c_str()))
                    {
                        metadata->Add(entity);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }
}
} // namespace Chained

#include "property_editor.h"
#include "engine/reflection/reflection_rfl.h"
#include "engine/scene/component_registry.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "editor/layer.h"
#include "editor/undo/component_commands.h"
#include "editor/undo/modify_component_command.h"
#include "engine/core/service_locator.h"
#include "gui.h"

#include "engine/physics/physics.h"
#include "engine/scene/scene_settings.h"
#include "imgui.h"
#include "ui_properties.h" // Included here to break circular dependency
#include <memory>
#include "scripting/scriptengine.h"
#include <Coral/ManagedObject.hpp>

#include "engine/app/application.h"
#include <yaml-cpp/yaml.h>
#include "engine/scene/component_registry.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"

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

template <typename T>
void PropertyEditor::RegisterComponentImpl(
    const std::string& name, const char* icon,
    std::function<void(Entity)> drawUI)
{
    auto typeId = entt::type_hash<T>::value();
    auto& metadata = ComponentRegistry::Exists(typeId)
        ? ComponentRegistry::GetMetadataMutable(typeId)
        : ([&]() -> ComponentMetadata& {
            ComponentMetadata fresh;
            fresh.Name = name;
            fresh.Icon = icon;
            fresh.Category = "Engine";
            fresh.SerializationKey = name + "Component";
            ComponentRegistry::Register(typeId, fresh);
            return ComponentRegistry::GetMetadataMutable(typeId);
          })();

    metadata.Name = name;
    metadata.Icon = icon;
    metadata.DrawUI = drawUI;
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<T>() && s_EditorLayer)
            s_EditorLayer->GetCommandHistory().PushCommand(
                std::make_unique<AddComponentCommand<T>>(e));
    };
    metadata.Remove = [](Entity e) {
        if (s_EditorLayer)
            s_EditorLayer->GetCommandHistory().PushCommand(
                std::make_unique<RemoveComponentCommand<T>>(e));
    };
}

template <typename T>
void PropertyEditor::Register(const std::string& name, const char* icon)
{
    RegisterComponentImpl<T>(name, icon,
        [name, icon](Entity e) { DrawComponentReflection<T>(name, icon, e); });
}

template <typename T>
void PropertyEditor::RegisterCustom(const std::string& name,
    std::function<bool(T&, Entity)> drawer, const char* icon)
{
    RegisterComponentImpl<T>(name, icon,
        [name, icon, drawer](Entity e) { DrawComponentContainer<T>(name, icon, e, drawer); });
}

// --- Implementation ---

void PropertyEditor::Init()
{
    s_EditorLayer = &EditorLayer::Get();

    // --- Core Components ---
    ComponentRegistry::SetAllowAdd(entt::type_hash<TransformComponent>::value(), false);
    RegisterCustom<LightComponent>("Light", [&](LightComponent& comp, Entity entity) {
        bool changed = false;
        UIProperties ui;
        Properties props(ui);

        int typeIdx = static_cast<int>(comp.Type);
        static const char* lightTypes[] = {"Point", "Spot", "Directional"};
        if (ui.Enum("Type", typeIdx, lightTypes, 3))
        {
            comp.Type = static_cast<LightType>(typeIdx);
            changed = true;
        }
        if (ui.Property("Color", comp.LightColor)) changed = true;
        if (ui.Property("Intensity", comp.Intensity, PropertyMeta(0.0f, 10000.0f, 5.0f))) changed = true;
        if (ui.Property("Range", comp.Radius, PropertyMeta(0.0f, 1000.0f, 1.0f))) changed = true;

        if (comp.Type == LightType::Spot)
        {
            if (ui.Property("Inner Cutoff", comp.InnerCutoff, PropertyMeta(0.0f, 90.0f, 0.5f))) changed = true;
            if (ui.Property("Outer Cutoff", comp.OuterCutoff, PropertyMeta(0.0f, 90.0f, 0.5f))) changed = true;
        }

        if (ui.Property("Cast Shadows", comp.Shadows)) changed = true;

        return changed;
    }, ICON_FA_LIGHTBULB);
    RegisterCustom<ColliderComponent>("Collider", [&](ColliderComponent& comp, Entity entity) {
        bool changed = false;
        UIProperties ui;
        Properties props(ui);

        int typeIdx = static_cast<int>(comp.Type);
        static const char* colliderTypes[] = {"Box", "Sphere", "Capsule", "Mesh"};
        if (ui.Enum("Type", typeIdx, colliderTypes, 4))
        {
            comp.Type = static_cast<ColliderType>(typeIdx);
            changed = true;
        }

        if (comp.Type == ColliderType::Box)
        {
            if (ui.Property("Size", comp.Size, PropertyMeta(0.01f, 100.0f, 0.05f))) changed = true;
        }
        else if (comp.Type == ColliderType::Sphere || comp.Type == ColliderType::Capsule)
        {
            if (ui.Property("Radius", comp.Radius, PropertyMeta(0.0f, 500.0f, 0.05f))) changed = true;
        }
        if (comp.Type == ColliderType::Capsule)
        {
            if (ui.Property("Height", comp.Height, PropertyMeta(0.0f, 500.0f, 0.05f))) changed = true;
        }

        if (ui.Property("Offset", comp.Offset, PropertyMeta(-10.0f, 10.0f, 0.05f))) changed = true;

        if (comp.Type == ColliderType::Mesh)
        {
            if (ui.Property("Auto Calculate", comp.AutoCalculate)) changed = true;
            if (!comp.AutoCalculate)
            {
                if (ui.File("Model Path", comp.ModelPath, ".glb,.gltf,.obj")) changed = true;
            }
        }

        if (ui.Property("Friction", comp.Friction, PropertyMeta(0.0f, 1.0f, 0.01f))) changed = true;
        if (ui.Property("Restitution", comp.Restitution, PropertyMeta(0.0f, 1.0f, 0.01f))) changed = true;
        if (ui.Property("Is Trigger", comp.IsTrigger)) changed = true;
        if (ui.Property("Enabled", comp.Enabled)) changed = true;

        return changed;
    }, ICON_FA_SHIELD);
    RegisterCustom<AnimationComponent>("Animation", [&](AnimationComponent& comp, Entity entity) {
        bool changed = false;
        UIProperties ui;
        Properties props(ui);
        
        if (ui.Property("Blend Duration", comp.BlendDuration, PropertyMeta(0.0f, 2.0f, 0.01f))) changed = true;
        if (ui.Property("Is Looping", comp.IsLooping)) changed = true;
        if (ui.Property("Play On Start", comp.PlayOnStart)) changed = true;
        if (ui.Property("Is Playing", comp.IsPlaying)) changed = true;

        if (entity.HasComponent<ModelComponent>())
        {
            auto& mc = entity.GetComponent<ModelComponent>();
            auto* am = ServiceLocator::Get<AssetManager>();
            if (mc.ModelHandle != 0 && am)
            {
                if (auto asset = am->Get<ModelAsset>(mc.ModelHandle))
                {
                    if (asset->GetAnimationCount() > 0)
                    {
                        std::vector<std::string> animNames;
                        std::vector<const char*> cStrs;
                        for (int i = 0; i < asset->GetAnimationCount(); i++)
                        {
                            std::string name = asset->GetAnimationName(i);
                            if (name.empty()) name = "Animation " + std::to_string(i);
                            animNames.push_back(name);
                        }
                        for (auto& s : animNames) cStrs.push_back(s.c_str());

                        ImGui::SetNextItemWidth(-1);
                        int currentIdx = comp.CurrentAnimationIndex;
                        if (ui.Enum("Current Animation", currentIdx, cStrs.data(), (int)cStrs.size()))
                        {
                            comp.CurrentAnimationIndex = currentIdx;
                            comp.CurrentFrame = 0;
                            comp.FrameTimeCounter = 0;
                            changed = true;
                        }

                        if (comp.CurrentAnimationIndex >= 0 && comp.CurrentAnimationIndex < (int)animNames.size())
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Name");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextDisabled("%s", animNames[comp.CurrentAnimationIndex].c_str());

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Playback");
                            ImGui::TableSetColumnIndex(1);
                            if (comp.IsPlaying)
                            {
                                if (ImGui::Button("Stop", ImVec2(-1, 0)))
                                {
                                    comp.IsPlaying = false;
                                    changed = true;
                                }
                            }
                            else
                            {
                                if (ImGui::Button("Play", ImVec2(-1, 0)))
                                {
                                    comp.IsPlaying = true;
                                    comp.CurrentFrame = 0;
                                    comp.FrameTimeCounter = 0;
                                    changed = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("No animations in model.");
                    }
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Requires ModelComponent");
        }

        return changed;
    }, ICON_FA_FILM);

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

    // --- UI Widgets ---
    RegisterCustom<UIControlComponent>("Widget", [](UIControlComponent& comp, Entity entity) {
        bool changed = false;
        UIProperties ui;

        // Box Style
        ui.Header("Box Style");
        if (ui.Property("BG Color",        comp.BoxStyle.BackgroundColor)) changed = true;
        if (ui.Property("Hover Color",     comp.BoxStyle.HoverColor))      changed = true;
        if (ui.Property("Pressed Color",   comp.BoxStyle.PressedColor))    changed = true;
        if (ui.Property("Border Color",    comp.BoxStyle.BorderColor))     changed = true;
        if (ui.Property("Rounding",        comp.BoxStyle.Rounding,        PropertyMeta(0.0f, 32.0f, 0.5f))) changed = true;
        if (ui.Property("Border Size",     comp.BoxStyle.BorderSize,      PropertyMeta(0.0f, 10.0f, 0.1f))) changed = true;
        if (ui.Property("Padding",         comp.BoxStyle.Padding,         PropertyMeta(0.0f, 64.0f, 0.5f))) changed = true;
        if (ui.Property("Hover Scale",     comp.BoxStyle.HoverScale,      PropertyMeta(0.5f, 3.0f,  0.01f))) changed = true;
        if (ui.Property("Pressed Scale",   comp.BoxStyle.PressedScale,    PropertyMeta(0.5f, 3.0f,  0.01f))) changed = true;
        if (ui.Property("Transition Speed",comp.BoxStyle.TransitionSpeed, PropertyMeta(0.0f, 2.0f,  0.01f))) changed = true;
        if (ui.Property("Gradient",        comp.BoxStyle.UseGradient))    changed = true;

        ui.Separator();
        // Text Style
        ui.Header("Text Style");
        if (ui.Property("Font Name",       comp.TextStyle.FontName))      changed = true;
        if (ui.Property("Font Size",       comp.TextStyle.FontSize,       PropertyMeta(4.0f, 256.0f, 0.5f))) changed = true;
        if (ui.Property("Text Color",      comp.TextStyle.TextColor))     changed = true;
        if (ui.Property("Shadow",          comp.TextStyle.Shadow))        changed = true;
        if (ui.Property("Letter Spacing",  comp.TextStyle.LetterSpacing,  PropertyMeta(0.0f, 10.0f, 0.05f))) changed = true;
        if (ui.Property("Line Height",     comp.TextStyle.LineHeight,     PropertyMeta(0.0f, 5.0f,  0.05f))) changed = true;

        ui.Separator();
        // Widget-type specific
        std::visit([&](auto&& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, ButtonData>) {
                if (ui.Property("Label",        data.Label))        changed = true;
                if (ui.Property("Interactable", data.IsInteractable)) changed = true;
                if (ui.Property("Auto Size",    data.AutoSize))      changed = true;
            } else if constexpr (std::is_same_v<T, LabelData>) {
                if (ui.Property("Text",         data.Text))         changed = true;
                if (ui.Property("Auto Size",    data.AutoSize))     changed = true;
            } else if constexpr (std::is_same_v<T, CheckboxData>) {
                if (ui.Property("Label",        data.Label))        changed = true;
                if (ui.Property("Checked",      data.Checked))      changed = true;
            } else if constexpr (std::is_same_v<T, SliderData>) {
                if (ui.Property("Label",        data.Label))        changed = true;
                if (ui.Property("Value",        data.Value,         PropertyMeta(data.Min, data.Max, 0.01f))) changed = true;
                if (ui.Property("Min",          data.Min))          changed = true;
                if (ui.Property("Max",          data.Max))          changed = true;
            } else if constexpr (std::is_same_v<T, ProgressBarData>) {
                if (ui.Property("Progress",     data.Progress,      PropertyMeta(0.0f, 1.0f, 0.01f))) changed = true;
                if (ui.Property("Overlay Text", data.OverlayText))  changed = true;
                if (ui.Property("Show %",       data.ShowPercentage)) changed = true;
            } else if constexpr (std::is_same_v<T, ImageData>) {
                if (ui.File("Texture Path",     data.TexturePath, ".png,.jpg,.jpeg,.bmp,.tga")) changed = true;
                if (ui.Property("Tint Color",   data.TintColor))    changed = true;
                if (ui.Property("Border Color", data.BorderColor))  changed = true;
            } else if constexpr (std::is_same_v<T, PanelData>) {
                if (ui.File("Texture Path",     data.TexturePath, ".png,.jpg,.jpeg"))  changed = true;
                if (ui.Property("Full Screen",  data.FullScreen))   changed = true;
            } else if constexpr (std::is_same_v<T, ComboBoxData>) {
                if (ui.Property("Label",        data.Label))        changed = true;
                if (ui.Property("Selected",     data.SelectedIndex, PropertyMeta(0, (int)data.Items.size()-1, 1))) changed = true;
                // Items list
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("Items");
                ImGui::TableSetColumnIndex(1);
                int removeIdx = -1;
                for (int i = 0; i < (int)data.Items.size(); i++) {
                    ImGui::PushID(i);
                    char buf[256]; strncpy(buf, data.Items[i].c_str(), sizeof(buf)-1); buf[sizeof(buf)-1]=0;
                    if (ImGui::InputText("##item", buf, sizeof(buf))) {
                        data.Items[i] = buf; changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton(ICON_FA_TRASH)) { removeIdx = i; changed = true; }
                    ImGui::PopID();
                }
                if (removeIdx >= 0) data.Items.erase(data.Items.begin() + removeIdx);
                if (ImGui::SmallButton(ICON_FA_PLUS " Add Item")) { data.Items.push_back(""); changed = true; }
            }
        }, comp.Data);

        return changed;
    }, ICON_FA_SHAPES);

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

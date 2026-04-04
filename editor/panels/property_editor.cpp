#include "property_editor.h"
#include "editor/editor_layer.h"
#include "editor_gui.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/physics/physics.h"
#include "ui_properties.h"
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

void PropertyEditor::RegisterComponent(entt::id_type typeId, const ComponentMetadata& metadata)
{
    s_ComponentRegistry[typeId] = metadata;
}

void PropertyEditor::Init()
{
#define REG_HIDDEN(T, name)                                                                                            \
    Register<T>(name, [](auto&, auto) { return false; });                                                              \
    s_ComponentRegistry[entt::type_hash<T>::value()].Visible = false;

#define REG_REFLECT(T, name)                                                                                           \
    Register<T>(name, [](auto& component, auto entity) {                                                               \
        CHEngine::UIProperties ui;                                                                                     \
        CHEngine::Properties props(ui);                                                                               \
        component.Reflect(props);                                                                                      \
        return props.HasChanged();                                                                                     \
    });

    // Core Components
    Register<TransformComponent>("Transform", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        if (props.HasChanged())
        {
            component.IsDirty = true;
            return true;
        }
        return false;
    });
    s_ComponentRegistry[entt::type_hash<TransformComponent>::value()].AllowAdd = false;

    REG_REFLECT(TagComponent, "Tag");
    REG_REFLECT(CameraComponent, "Camera");
    REG_REFLECT(LightComponent, "Light");
    REG_REFLECT(RigidBodyComponent, "RigidBody");
    REG_REFLECT(ColliderComponent, "Collider");
    REG_REFLECT(ModelComponent, "Model");
    REG_REFLECT(MaterialComponent, "Materials");
    REG_REFLECT(SpriteComponent, "Sprite");
    REG_REFLECT(PrimitiveComponent, "Primitive");
    REG_REFLECT(ShaderComponent, "Shader");
    REG_REFLECT(AnimationComponent, "Animation");
    REG_REFLECT(AudioComponent, "Audio");
    REG_REFLECT(SpawnComponent, "Spawn Zone");
    REG_REFLECT(PlayerComponent, "Player");
    REG_REFLECT(SceneTransitionComponent, "Scene Transition");
    Register<ManagedScriptComponent>("Scripts", [](auto& component, auto entity) {
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
    });
    
    REG_REFLECT(ControlComponent, "Rect Transform");
    s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = true;

    // UI Navigation (Still custom due to entity selection)
    Register<NavigationComponent>("UI Navigation", [](auto& component, auto entity) {
        bool changed = false;
        auto pb = EditorGUI::Begin();
        pb.Bool("Default Focus", component.IsDefaultFocus);
        
        int up = (int)(uint32_t)component.Up;
        int down = (int)(uint32_t)component.Down;
        int left = (int)(uint32_t)component.Left;
        int right = (int)(uint32_t)component.Right;

        pb.Int("Up (ID)", up).Int("Down (ID)", down).Int("Left (ID)", left).Int("Right (ID)", right);
        if (pb.Changed)
        {
            component.Up = (entt::entity)up;
            component.Down = (entt::entity)down;
            component.Left = (entt::entity)left;
            component.Right = (entt::entity)right;
            changed = true;
        }

        return changed;
    });

    REG_REFLECT(UIActionComponent, "UI Action");

    // UI Widgets
#define REG_WIDGET(T, name) \
    Register<T>(name, [](auto& component, auto entity) { \
        CHEngine::UIProperties ui; \
        CHEngine::Properties props(ui); \
        component.Reflect(props); \
        return props.HasChanged(); \
    }); \
    s_ComponentRegistry[entt::type_hash<T>::value()].IsWidget = true; \
    s_ComponentRegistry[entt::type_hash<T>::value()].AllowAdd = true;

    REG_WIDGET(ButtonControl, "Button Widget");
    REG_WIDGET(PanelControl, "Panel Widget");
    REG_WIDGET(LabelControl, "Label Widget");
    REG_WIDGET(SliderControl, "Slider Widget");
    REG_WIDGET(CheckboxControl, "Checkbox Widget");
    REG_WIDGET(InputTextControl, "Input Text Widget");
    REG_WIDGET(ComboBoxControl, "ComboBox Widget");
    REG_WIDGET(ProgressBarControl, "ProgressBar Widget");
    REG_WIDGET(ImageControl, "Image Widget");
    REG_WIDGET(ImageButtonControl, "Image Button Widget");
    REG_WIDGET(SeparatorControl, "Separator Widget");
    REG_WIDGET(RadioButtonControl, "RadioButton Widget");
    REG_WIDGET(ColorPickerControl, "ColorPicker Widget");
    REG_WIDGET(DragFloatControl, "DragFloat Widget");
    REG_WIDGET(DragIntControl, "DragInt Widget");
    REG_WIDGET(TabBarControl, "TabBar Widget");
    REG_WIDGET(TabItemControl, "Tab Item Widget");
    REG_WIDGET(CollapsingHeaderControl, "CollapsingHeader Widget");
    REG_WIDGET(VerticalLayoutGroup, "Vertical Layout Group");
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

void PropertyEditor::DrawTag(CHEngine::Entity entity)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto& tag = entity.GetComponent<TagComponent>().Tag;
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);

        ImGui::Text("Tag");
        ImGui::SameLine();
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tag = std::string(buffer);
        }
    }
}

static bool DrawTextureSlot(const char* label, std::string& path)
{
    bool changed = false;
    float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImVec2 slotSize = { 48, 48 };

    ImGui::PushID(label);
    
    // Label column if we are not in a table (fallback)
    if (ImGui::GetColumnsCount() <= 1 && !ImGui::GetCurrentTable())
    {
        ImGui::Text("%s", label);
        ImGui::SameLine(100.0f);
    }

    // Texture Slot Rectangle
    ImGui::BeginGroup();
    
    ImTextureID texID = (ImTextureID)(intptr_t)0; // Placeholder
    bool hasTex = false;

    if (!path.empty())
    {
        auto textureAsset = AssetManager::Get().Get<TextureAsset>(path);
        if (textureAsset && textureAsset->GetTexture() && textureAsset->GetTexture()->IsReady())
        {
            texID = (ImTextureID)(intptr_t)textureAsset->GetTexture()->GetRendererID();
            hasTex = true;
        }
    }

    // Draw background placeholder if no texture
    if (!hasTex)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p, {p.x + slotSize.x, p.y + slotSize.y}, ImGui::GetColorU32(ImGuiCol_FrameBg));
        ImGui::GetWindowDrawList()->AddRect(p, {p.x + slotSize.x, p.y + slotSize.y}, ImGui::GetColorU32(ImGuiCol_Border));
        
        // Draw a tiny '+' in the middle
        float center_x = p.x + slotSize.x * 0.5f;
        float center_y = p.y + slotSize.y * 0.5f;
        ImGui::GetWindowDrawList()->AddText({center_x - 5, center_y - 7}, ImGui::GetColorU32(ImGuiCol_TextDisabled), ICON_FA_PLUS);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1, 1, 1, 0.05f});
    if (ImGui::ImageButton("##slot", texID, slotSize, {0, 1}, {1, 0}))
    {
        // Maybe open a custom picker? For now just browse
        std::vector<FileDialogFilter> filters = {{"Images", "png,jpg,tga,bmp"}};
        auto result = Dialogs::OpenFile(filters);
        if (result)
        {
            std::filesystem::path p = *result;
            auto projectPath = Project::GetAssetDirectory();
            std::filesystem::path relativePath = std::filesystem::relative(p, projectPath);
            path = relativePath.empty() ? p.string() : relativePath.string();
            changed = true;
        }
    }
    ImGui::PopStyleColor(2);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* dropPath = (const char*)payload->Data;
            std::filesystem::path p = dropPath;
            auto projectPath = Project::GetAssetDirectory();
            std::filesystem::path relativePath = std::filesystem::relative(p, projectPath);
            path = relativePath.empty() ? p.string() : relativePath.string();
            changed = true;
        }
        ImGui::EndDragDropTarget();
    }

    // Hover tooltip
    if (hasTex)
    {
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Image(texID, {256, 256}, {0, 1}, {1, 0});
            ImGui::Text("%s", path.c_str());
            ImGui::EndTooltip();
        }
    }

    // Filename and Clear button overlay
    ImGui::SameLine();
    ImGui::BeginGroup();
    std::string filename = path.empty() ? "None" : std::filesystem::path(path).filename().string();
    if (filename.length() > 15) filename = filename.substr(0, 12) + "...";
    
    ImGui::TextDisabled("%s", filename.c_str());
    
    if (!path.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.3f, 0.1f, 0.1f, 1.0f});
        if (ImGui::Button(ICON_FA_XMARK " Clear"))
        {
            path = "";
            changed = true;
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndGroup();

    ImGui::EndGroup();
    ImGui::PopID();

    return changed;
}

static std::string GetTexturePathFromID(uint32_t id, const std::vector<std::shared_ptr<CHEngine::TextureAsset>>& textures)
{
    if (id == 0)
        return "";

    for (const auto& tex : textures)
    {
        if (tex->GetID() == id)
            return tex->GetPath();
    }
    return "";
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

            // Filtering: Only show widgets if the entity is a UI entity
            // (or if it's the ControlComponent itself which can be added to any transform)
            if (metadata.IsWidget && !isUIEntity)
            {
                continue;
            }

            // [FIX] Constraint: Do not allow adding UI components/widgets in 3D (Environment3D) scenes
            if (is3DScene)
            {
                if (metadata.IsWidget || id == entt::type_hash<ControlComponent>::value())
                {
                    continue;
                }
            }

            auto& registry = entity.GetRegistry();
            auto* storage = registry.storage(id);
            if (storage && storage->contains(entity))
            {
                continue;
            }

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

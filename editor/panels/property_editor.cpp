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

    Register<CameraComponent>("Camera", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<LightComponent>("Light", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        
        if (component.Radius <= 0.01f)
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::NextColumn();
            ImGui::TextColored({ 1, 1, 0, 1 }, ICON_FA_CIRCLE_EXCLAMATION " Radius is 0");
            ImGui::Columns(1);
        }

        return props.HasChanged();
    });

    Register<RigidBodyComponent>("RigidBody", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<ColliderComponent>("Collider", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);

        if (component.Type == ColliderType::Mesh)
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::NextColumn();
            if (ImGui::Button(ICON_FA_HAMMER " Rebuild Body", {-1, 0}))
            {
                auto asset = AssetManager::Get().Get<ModelAsset>(component.ModelPath);
                if (asset && asset->IsReady())
                {
                    if (component.AutoCalculate)
                    {
                        auto box = asset->GetBoundingBox();
                        component.Offset = box.Min; 
                        component.Size = box.Max - box.Min;
                    }
                    PhysicsSystem::Get().InvalidateBVH(component.ModelPath);
                }
                return true;
            }
            ImGui::Columns(1);
        }

        return props.HasChanged();
    });

    Register<TagComponent>("Tag", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<ShaderComponent>("Shader", [](auto& component, auto entity) {
        // Shaders still use custom UI due to Uniforms complexity and ShaderLibrary integration
        bool changed = false;

        if (Renderer::IsInitialized())
        {
            auto& lib = Renderer::Get().GetShaderLibrary();
            std::vector<std::string> names = lib.GetNames();
            std::sort(names.begin(), names.end());

            std::string currentName = "Custom";
            for (const auto& name : names)
            {
                if (lib.Get(name)->GetPath() == component.ShaderPath)
                {
                    currentName = name;
                    break;
                }
            }

            EditorGUI::BeginProperty("Shader");
            if (ImGui::BeginCombo("##ShaderCombo", currentName.c_str()))
            {
                if (ImGui::Selectable("Custom", currentName == "Custom")) {}
                for (const auto& name : names)
                {
                    if (ImGui::Selectable(name.c_str(), currentName == name))
                    {
                        component.ShaderPath = lib.Get(name)->GetPath();
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            EditorGUI::EndProperty();
        }

        if (EditorGUI::Property("Shader Path", component.ShaderPath, "chshader")) changed = true;
        if (EditorGUI::Property("Enabled", component.Enabled)) changed = true;

        if (!component.Uniforms.empty() && ImGui::TreeNodeEx("Uniforms", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& u : component.Uniforms)
            {
                EditorGUI::BeginProperty(u.Name.c_str());
                if (u.Type == 0) { if (ImGui::DragFloat("##U", &u.Value[0], 0.05f)) changed = true; }
                else if (u.Type == 1) { if (ImGui::DragFloat2("##U", u.Value, 0.05f)) changed = true; }
                else if (u.Type == 2) { if (ImGui::DragFloat3("##U", u.Value, 0.05f)) changed = true; }
                else if (u.Type == 4) { if (ImGui::ColorEdit4("##U", u.Value)) changed = true; }
                EditorGUI::EndProperty();
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        ImGui::NextColumn();
        if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Sync Uniforms", {-1, 0}))
        {
            std::string fullPath = AssetManager::Get().ResolvePath(component.ShaderPath);
            if (std::filesystem::exists(fullPath))
            {
                try {
                    YAML::Node config = YAML::LoadFile(fullPath);
                    if (config["Uniforms"])
                    {
                        std::vector<ShaderUniform> newUniforms;
                        for (auto uNode : config["Uniforms"])
                        {
                            std::string name = uNode.as<std::string>();
                            auto it = std::find_if(component.Uniforms.begin(), component.Uniforms.end(), [&](const auto& e) { return e.Name == name; });
                            if (it != component.Uniforms.end()) newUniforms.push_back(*it);
                            else { ShaderUniform u; u.Name = name; u.Type = name.find("Color") != std::string::npos ? 4 : 0; newUniforms.push_back(u); }
                        }
                        component.Uniforms = newUniforms;
                        changed = true;
                    }
                } catch (...) {}
            }
        }
        ImGui::Columns(1);
        return changed;
    });

    Register<AudioComponent>("Audio", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        
        ImGui::Separator();
        if (ImGui::Button("Play")) { component.IsPlaying = true; }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) { component.IsPlaying = false; }

        return props.HasChanged();
    });

    Register<SpawnComponent>("Spawn Zone", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<PlayerComponent>("Player", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<SceneTransitionComponent>("Scene Transition", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<AnimationComponent>("Animation", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);

        int animCount = 0;
        std::shared_ptr<ModelAsset> modelAsset;
        if (entity.template HasComponent<ModelComponent>())
        {
            auto& mc = entity.template GetComponent<ModelComponent>();
            modelAsset = AssetManager::Get().Get<ModelAsset>(mc.ModelPath);
            if (modelAsset) animCount = modelAsset->GetAnimationCount();
        }

        if (animCount > 0)
        {
            std::string currentAnimName = modelAsset->GetAnimationName(component.CurrentAnimationIndex);
            EditorGUI::BeginProperty("Animation Select");
            if (ImGui::BeginCombo("##AnimCombo", currentAnimName.c_str()))
            {
                for (int i = 0; i < animCount; i++)
                {
                    bool isSelected = (component.CurrentAnimationIndex == i);
                    if (ImGui::Selectable(modelAsset->GetAnimationName(i).c_str(), isSelected))
                    {
                        component.CurrentAnimationIndex = i;
                        props.SetChanged(true);
                    }
                }
                ImGui::EndCombo();
            }
            EditorGUI::EndProperty();
        }

        return props.HasChanged();
    });

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

    Register<ModelComponent>("Model", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<MaterialComponent>("Materials", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });


    Register<SpriteComponent>("Sprite", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<PrimitiveComponent>("Primitive", [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<ManagedScriptComponent>("Scripts", [](auto& component, Entity entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    });

    Register<UIActionComponent>("UI Action", [](auto& component, auto entity) {
        auto pb = EditorGUI::Begin();
        std::string uuidStr = component.TargetEntityID.ToString();
        if (EditorGUI::Property("Target UUID", uuidStr))
        {
            component.TargetEntityID = UUID(uuidStr);
            pb.Changed = true;
        }
        pb.String("Parameter", component.ParameterName).Float("Value", component.Value);
        return pb.Changed;
    });

    // --- UI Widgets ---
    Register<ControlComponent>("Rect Transform", [](auto& component, auto entity) {
        auto& rectTransform = component.Transform;
        bool changed = false;

        // --- Anchor Presets ---
        ImGui::Text("Presets:");
        ImGui::SameLine();
        if (ImGui::Button("Center"))
        {
            rectTransform.AnchorMin = {0.5f, 0.5f};
            rectTransform.AnchorMax = {0.5f, 0.5f};
            rectTransform.OffsetMin = {-50, -50};
            rectTransform.OffsetMax = {50, 50};
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stretch"))
        {
            rectTransform.AnchorMin = {0.0f, 0.0f};
            rectTransform.AnchorMax = {1.0f, 1.0f};
            rectTransform.OffsetMin = {0, 0};
            rectTransform.OffsetMax = {0, 0};
            changed = true;
        }

        bool isPoint = (rectTransform.AnchorMin.x == rectTransform.AnchorMax.x &&
                        rectTransform.AnchorMin.y == rectTransform.AnchorMax.y);
        if (isPoint)
        {
            float width = rectTransform.OffsetMax.x - rectTransform.OffsetMin.x;
            float height = rectTransform.OffsetMax.y - rectTransform.OffsetMin.y;
            float posX = rectTransform.OffsetMin.x + width * rectTransform.Pivot.x;
            float posY = rectTransform.OffsetMin.y + height * rectTransform.Pivot.y;

            glm::vec2 pos = {posX, posY};
            glm::vec2 size = {width, height};

            if (EditorGUI::Property("Pos", pos))
            {
                rectTransform.OffsetMin.x = pos.x - size.x * rectTransform.Pivot.x;
                rectTransform.OffsetMin.y = pos.y - size.y * rectTransform.Pivot.y;
                rectTransform.OffsetMax.x = pos.x + size.x * (1.0f - rectTransform.Pivot.x);
                rectTransform.OffsetMax.y = pos.y + size.y * (1.0f - rectTransform.Pivot.y);
                changed = true;
            }
            if (EditorGUI::Property("Size", size))
            {
                rectTransform.OffsetMin.x = pos.x - size.x * rectTransform.Pivot.x;
                rectTransform.OffsetMin.y = pos.y - size.y * rectTransform.Pivot.y;
                rectTransform.OffsetMax.x = pos.x + size.x * (1.0f - rectTransform.Pivot.x);
                rectTransform.OffsetMax.y = pos.y + size.y * (1.0f - rectTransform.Pivot.y);
                changed = true;
            }
        }
        else
        {
            float rightPadding = -rectTransform.OffsetMax.x;
            float bottomPadding = -rectTransform.OffsetMax.y;

            if (EditorGUI::Property("Left", rectTransform.OffsetMin.x))
            {
                changed = true;
            }
            if (EditorGUI::Property("Top", rectTransform.OffsetMin.y))
            {
                changed = true;
            }
            if (EditorGUI::Property("Right", rightPadding))
            {
                rectTransform.OffsetMax.x = -rightPadding;
                changed = true;
            }
            if (EditorGUI::Property("Bottom", bottomPadding))
            {
                rectTransform.OffsetMax.y = -bottomPadding;
                changed = true;
            }
        }

        if (ImGui::TreeNodeEx("Advanced Layout Settings", ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            if (EditorGUI::Property("Pivot", rectTransform.Pivot))
            {
                changed = true;
            }
            if (EditorGUI::Property("Anchor Min", rectTransform.AnchorMin))
            {
                changed = true;
            }
            if (EditorGUI::Property("Anchor Max", rectTransform.AnchorMax))
            {
                changed = true;
            }
            if (EditorGUI::Property("Rotation", rectTransform.Rotation))
            {
                changed = true;
            }
            if (EditorGUI::Property("Scale", rectTransform.Scale))
            {
                changed = true;
            }
            if (EditorGUI::Property("Z Order", component.ZOrder))
            {
                changed = true;
            }
            if (EditorGUI::Property("Visible", component.IsActive))
            {
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    });

    auto stdReflect = [](auto& component, auto entity) {
        CHEngine::UIProperties ui;
        CHEngine::Properties props(ui);
        component.Reflect(props);
        return props.HasChanged();
    };

    Register<ButtonControl>("Button Widget", stdReflect);
    Register<PanelControl>("Panel Widget", stdReflect);
    Register<LabelControl>("Label Widget", stdReflect);
    Register<SliderControl>("Slider Widget", stdReflect);
    Register<CheckboxControl>("Checkbox Widget", stdReflect);
    Register<InputTextControl>("Input Text Widget", stdReflect);
    Register<ComboBoxControl>("ComboBox Widget", stdReflect);
    Register<ProgressBarControl>("ProgressBar Widget", stdReflect);
    Register<ImageControl>("Image Widget", stdReflect);
    Register<ImageButtonControl>("Image Button Widget", stdReflect);
    Register<SeparatorControl>("Separator Widget", stdReflect);
    Register<RadioButtonControl>("RadioButton Widget", stdReflect);
    Register<ColorPickerControl>("ColorPicker Widget", stdReflect);
    Register<DragFloatControl>("DragFloat Widget", stdReflect);
    Register<DragIntControl>("DragInt Widget", stdReflect);
    Register<TabBarControl>("TabBar Widget", stdReflect);
    Register<TabItemControl>("Tab Item Widget", stdReflect);
    Register<CollapsingHeaderControl>("CollapsingHeader Widget", stdReflect);
    Register<VerticalLayoutGroup>("Vertical Layout Group", stdReflect);

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
    setupWidget(entt::type_hash<TabBarControl>::value());
    setupWidget(entt::type_hash<TabItemControl>::value());
    setupWidget(entt::type_hash<CollapsingHeaderControl>::value());

    // Allow adding Rect Transform directly too
    s_ComponentRegistry[entt::type_hash<ControlComponent>::value()].AllowAdd = true;
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


void PropertyEditor::DrawMaterial(CHEngine::Entity entity, int hitMeshIndex)
{
    if (!entity.HasComponent<ModelComponent>())
        return;

    auto& mc = entity.GetComponent<ModelComponent>();
    auto mcAsset = AssetManager::Get().Get<ModelAsset>(mc.ModelPath);
    if (!mcAsset || mcAsset->GetState() != AssetState::Ready)
        return;

    const Model& model = mcAsset->GetModel();
    auto modelTextures = mcAsset->GetPendingData().materials;

    // Helper to draw a single material instance
    auto DrawMaterialInstance = [&](MaterialInstance& mat, int index, bool isOverride) {
        std::string header = (isOverride ? ICON_FA_PEN_TO_SQUARE " " : ICON_FA_LOCK " ") + 
                             std::string("Material ") + std::to_string(index) + (isOverride ? " (Override)" : " (Default)");
        
        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(index);
            ImGui::BeginDisabled(!isOverride);

            if (ImGui::BeginTable("MaterialProperties", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                // --- Albedo Section ---
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Albedo");
                
                ImGui::TableSetColumnIndex(1);
                ImGui::PushID("Albedo");
                if (EditorGUI::Property("Color", mat.AlbedoColor)) mat.OverrideAlbedo = true;
                if (DrawTextureSlot("Map", mat.AlbedoPath)) mat.OverrideAlbedo = true;
                ImGui::PopID();
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Separator();
                ImGui::TableSetColumnIndex(1);
                ImGui::Separator();

                // --- PBR Section ---
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("PBR Maps");
                
                ImGui::TableSetColumnIndex(1);
                DrawTextureSlot("Normal", mat.NormalMapPath);
                DrawTextureSlot("Metallic/Roughness", mat.MetallicRoughnessPath);
                DrawTextureSlot("Occlusion", mat.OcclusionMapPath);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Separator();
                ImGui::TableSetColumnIndex(1);
                ImGui::Separator();

                // --- Parameters Section ---
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Parameters");

                ImGui::TableSetColumnIndex(1);
                EditorGUI::Property("Metalness", mat.Metalness, 0.01f, 0.0f, 1.0f);
                EditorGUI::Property("Roughness", mat.Roughness, 0.01f, 0.0f, 1.0f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Separator();
                ImGui::TableSetColumnIndex(1);
                ImGui::Separator();

                // --- Emissive Section ---
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Emissive");

                ImGui::TableSetColumnIndex(1);
                ImGui::PushID("Emissive");
                if (EditorGUI::Property("Color", mat.EmissiveColor)) mat.OverrideEmissive = true;
                EditorGUI::Property("Intensity", mat.EmissiveIntensity, 0.1f, 0.0f, 100.0f);
                DrawTextureSlot("Map", mat.EmissivePath);
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Separator();
                ImGui::TableSetColumnIndex(1);
                ImGui::Separator();

                // --- Rendering Section ---
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Rendering");

                ImGui::TableSetColumnIndex(1);
                EditorGUI::Property("Double Sided", mat.DoubleSided);
                EditorGUI::Property("Transparent", mat.Transparent);
                if (mat.Transparent)
                    EditorGUI::Property("Alpha", mat.Alpha, 0.01f, 0.0f, 1.0f);

                ImGui::EndTable();
            }

            ImGui::EndDisabled();

            if (!isOverride)
            {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.4f, 0.2f, 1.0f});
                if (ImGui::Button(ICON_FA_PLUS " Create Override from Defaults", {-1, 0}))
                {
                    MaterialSlot newSlot;
                    newSlot.Name = "Override " + std::to_string(index);
                    newSlot.Target = MaterialSlotTarget::MaterialIndex;
                    newSlot.Index = index;
                    newSlot.Material = mat; 
                    mc.Materials.push_back(newSlot);
                }
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }
    };

    if (hitMeshIndex >= 0 && hitMeshIndex < (int)model.Meshes.size())
    {
        int matIndex = model.Meshes[hitMeshIndex].MaterialIndex;
        int slotIndex = -1;

        // 1. Check for Mesh Index override
        for (int i = 0; i < (int)mc.Materials.size(); i++)
        {
            if (mc.Materials[i].Target == MaterialSlotTarget::MeshIndex && mc.Materials[i].Index == hitMeshIndex)
            {
                slotIndex = i;
                break;
            }
        }

        // 2. Check for Material Index override
        if (slotIndex == -1)
        {
            for (int i = 0; i < (int)mc.Materials.size(); i++)
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
            DrawMaterialInstance(mc.Materials[slotIndex].Material, slotIndex, true);
        }
        else
        {
            // Synthesis default material info for display
            MaterialInstance defaultMat;
            const Material& rMat = model.Materials[matIndex];
            
            // defaultMat.AlbedoColor = { (unsigned char)(rMat.AlbedoColor.r * 255), (unsigned char)(rMat.AlbedoColor.g * 255), (unsigned char)(rMat.AlbedoColor.b * 255), (unsigned char)(rMat.AlbedoColor.a * 255) };
            // defaultMat.AlbedoPath = GetTexturePathFromID(rMat.AlbedoMap, modelTextures);
            // defaultMat.OverrideAlbedo = !defaultMat.AlbedoPath.empty();

            // defaultMat.NormalMapPath = GetTexturePathFromID(rMat.NormalMap, modelTextures);
            // defaultMat.MetallicRoughnessPath = GetTexturePathFromID(rMat.MetallicRoughnessMap, modelTextures);
            // defaultMat.OcclusionMapPath = GetTexturePathFromID(rMat.OcclusionMap, modelTextures);
            // defaultMat.EmissivePath = GetTexturePathFromID(rMat.EmissiveMap, modelTextures);
            // defaultMat.EmissiveColor = { (unsigned char)(rMat.EmissiveColor.r * 255), (unsigned char)(rMat.EmissiveColor.g * 255), (unsigned char)(rMat.EmissiveColor.b * 255), (unsigned char)(rMat.EmissiveColor.a * 255) };
            // defaultMat.EmissiveIntensity = rMat.EmissiveIntensity;
            // defaultMat.Metalness = rMat.Metalness;
            // defaultMat.Roughness = rMat.Roughness;

            DrawMaterialInstance(defaultMat, matIndex, false);
            
            ImGui::Separator();
            if (ImGui::Button("Create Mesh Specific Override"))
            {
                MaterialSlot newSlot;
                newSlot.Name = "Mesh Override " + std::to_string(hitMeshIndex);
                newSlot.Target = MaterialSlotTarget::MeshIndex;
                newSlot.Index = hitMeshIndex;
                newSlot.Material = defaultMat;
                mc.Materials.push_back(newSlot);
            }
        }
    }
    else
    {
        // Show all materials of the model first
        for (int m = 0; m < (int)model.Materials.size(); m++)
        {
            int slotIdx = -1;
            for (int i = 0; i < (int)mc.Materials.size(); i++)
            {
                if (mc.Materials[i].Target == MaterialSlotTarget::MaterialIndex && mc.Materials[i].Index == m)
                {
                    slotIdx = i;
                    break;
                }
            }

            if (slotIdx != -1)
            {
                DrawMaterialInstance(mc.Materials[slotIdx].Material, slotIdx, true);
            }
            else
            {
                MaterialInstance defaultMat;
                const Material& rMat = model.Materials[m];
                
                // defaultMat.AlbedoColor = { (unsigned char)(rMat.AlbedoColor.r * 255), (unsigned char)(rMat.AlbedoColor.g * 255), (unsigned char)(rMat.AlbedoColor.b * 255), (unsigned char)(rMat.AlbedoColor.a * 255) };
                // defaultMat.AlbedoPath = GetTexturePathFromID(rMat.AlbedoMap, modelTextures);
                // defaultMat.OverrideAlbedo = !defaultMat.AlbedoPath.empty();
                
                // defaultMat.NormalMapPath = GetTexturePathFromID(rMat.NormalMap, modelTextures);
                // defaultMat.MetallicRoughnessPath = GetTexturePathFromID(rMat.MetallicRoughnessMap, modelTextures);
                // defaultMat.OcclusionMapPath = GetTexturePathFromID(rMat.OcclusionMap, modelTextures);
                // defaultMat.EmissivePath = GetTexturePathFromID(rMat.EmissiveMap, modelTextures);
                // defaultMat.EmissiveColor = { (unsigned char)(rMat.EmissiveColor.r * 255), (unsigned char)(rMat.EmissiveColor.g * 255), (unsigned char)(rMat.EmissiveColor.b * 255), (unsigned char)(rMat.EmissiveColor.a * 255) };
                // defaultMat.EmissiveIntensity = rMat.EmissiveIntensity;
                // defaultMat.Metalness = rMat.Metalness;
                // defaultMat.Roughness = rMat.Roughness;

                DrawMaterialInstance(defaultMat, m, false);
            }
        }
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

#include "material_panel.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "property_editor.h"
#include "ui_properties.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"

namespace Chained
{

MaterialPanel::MaterialPanel()
{
    m_Name = "Material Editor";
}

static uint32_t GetTextureID(const std::string& path)
{
    if (path.empty()) return 0;
    auto texAsset = ServiceLocator::Get<AssetManager>()->Get<TextureAsset>(path);
    if (texAsset && texAsset->GetTexture())
    {
        return texAsset->GetTexture()->GetNativeHandle();
    }
    return 0;
}

void MaterialPanel::DrawMaterialSlot(Material& mat)
{
    if (ImGui::CollapsingHeader(ICON_FA_IMAGE " Albedo", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditorGUI::Property("Color", mat.AlbedoColor);
        EditorGUI::FileProperty("Texture", mat.AlbedoPath, GetTextureID(mat.AlbedoPath), "png,jpg,tga");
    }

    if (ImGui::CollapsingHeader(ICON_FA_WATER " Normals", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditorGUI::FileProperty("Normal Map", mat.NormalPath, GetTextureID(mat.NormalPath), "png,jpg,tga");
    }

    if (ImGui::CollapsingHeader(ICON_FA_CIRCLE_HALF_STROKE " PBR (Metal/Rough)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, 100.0f);
        
        ImGui::Text("Metalness"); ImGui::NextColumn();
        ImGui::SliderFloat("##metal", &mat.Metalness, 0.0f, 1.0f); ImGui::NextColumn();
        
        ImGui::Text("Roughness"); ImGui::NextColumn();
        ImGui::SliderFloat("##rough", &mat.Roughness, 0.0f, 1.0f); ImGui::NextColumn();
        
        ImGui::Columns(1);
        
        EditorGUI::FileProperty("PBR Map", mat.MetallicRoughnessPath, GetTextureID(mat.MetallicRoughnessPath), "png,jpg,tga");
    }

    if (ImGui::CollapsingHeader(ICON_FA_SUN " Emissive", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditorGUI::Property("Emissive Color", mat.EmissiveColor);
        EditorGUI::Property("Intensity", mat.EmissiveIntensity);
        EditorGUI::FileProperty("Emissive Map", mat.EmissivePath, GetTextureID(mat.EmissivePath), "png,jpg,tga");
    }

    if (ImGui::CollapsingHeader(ICON_FA_GEARS " Settings"))
    {
        ImGui::Checkbox("Transparent", &mat.Transparent);
        ImGui::SliderFloat("Alpha", &mat.Alpha, 0.0f, 1.0f);
    }
}

void MaterialPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen) return;

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (m_SelectedEntity && (!m_SelectedEntity.IsValid() || m_SelectedEntity.GetRegistry().ctx().get<Scene*>() != m_Context.get()))
    {
        m_SelectedEntity = {};
    }

    if (m_SelectedEntity && m_SelectedEntity.IsValid())
    {
        ImGui::BeginDisabled(readOnly);

        std::vector<Material>* materials = nullptr;

        if (m_SelectedEntity.HasComponent<ModelComponent>())
        {
            auto& mc = m_SelectedEntity.GetComponent<ModelComponent>();
            auto asset = ServiceLocator::Get<AssetManager>()->Get<ModelAsset>(mc.ModelPath);
            if (asset)
            {
                materials = &asset->GetMaterials();
            }
        }

        if (materials && !materials->empty())
        {
            // Material Selection Sidebar / List
            ImGui::BeginChild("MaterialList", ImVec2(150, 0), true);
            for (int i = 0; i < (int)materials->size(); i++)
            {
                std::string label = (*materials)[i].Name;
                if (label.empty()) label = "Material " + std::to_string(i);
                
                if (ImGui::Selectable(label.c_str(), m_SelectedMaterialIndex == i))
                    m_SelectedMaterialIndex = i;
            }
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            // Material Properties
            ImGui::BeginChild("MaterialProperties");
            if (m_SelectedMaterialIndex < (int)materials->size())
            {
                ImGui::TextColored({0.2f, 0.8f, 1.0f, 1.0f}, "Editing: %s", (*materials)[m_SelectedMaterialIndex].Name.c_str());
                ImGui::Separator();
                DrawMaterialSlot((*materials)[m_SelectedMaterialIndex]);
            }
            else
            {
                m_SelectedMaterialIndex = 0;
            }
            ImGui::EndChild();
        }
        else
        {
            ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.0f}, ICON_FA_CIRCLE_INFO " No Materials found for this entity");
        }

        ImGui::EndDisabled();
    }
    else
    {
        ImGui::Text("No entity selected.");
        ImGui::TextDisabled("Select an entity in the Hierarchy to edit its materials.");
    }

    ImGui::End();
}

void MaterialPanel::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<EntitySelectedEvent>([this](EntitySelectedEvent& ev) {
        m_SelectedEntity = Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry());
        m_SelectedMeshIndex = ev.GetMeshIndex();
        m_SelectedMaterialIndex = 0;
        return false;
    });
}

void MaterialPanel::SetContext(const std::shared_ptr<Scene>& context)
{
    if (m_Context != context)
    {
        Panel::SetContext(context);
        m_SelectedEntity = {};
    }
}

} // namespace Chained

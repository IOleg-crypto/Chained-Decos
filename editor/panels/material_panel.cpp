#include "material_panel.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "property_editor.h"
#include "ui_properties.h"
#include "engine/graphics/texture_system.h"

namespace CHEngine
{

MaterialPanel::MaterialPanel()
{
    m_Name = "Material Editor";
}

static uint32_t GetTextureID(const std::string& path)
{
    if (path.empty()) return 0;
    auto textureHandle = TextureSystem::Get().LoadTexture(path);
    return TextureSystem::Get().GetRendererID(textureHandle);
}

void MaterialPanel::DrawMaterialSlot(MaterialSlot& slot)
{
    MaterialInstance& mat = slot.Material;
    
    if (ImGui::CollapsingHeader(ICON_FA_IMAGE " Albedo", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditorGUI::Property("Color", mat.AlbedoColor);
        mat.OverrideAlbedo |= EditorGUI::FileProperty("Texture", mat.AlbedoPath, GetTextureID(mat.AlbedoPath), "png,jpg,tga");
        ImGui::Checkbox("Override Albedo", &mat.OverrideAlbedo);
    }

    if (ImGui::CollapsingHeader(ICON_FA_WATER " Normals", ImGuiTreeNodeFlags_DefaultOpen))
    {
        mat.OverrideNormal |= EditorGUI::FileProperty("Normal Map", mat.NormalMapPath, GetTextureID(mat.NormalMapPath), "png,jpg,tga");
        ImGui::Checkbox("Override Normal", &mat.OverrideNormal);
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
        
        mat.OverrideMetallicRoughness |= EditorGUI::FileProperty("PBR Map", mat.MetallicRoughnessPath, GetTextureID(mat.MetallicRoughnessPath), "png,jpg,tga");
        ImGui::Checkbox("Override PBR", &mat.OverrideMetallicRoughness);
    }

    if (ImGui::CollapsingHeader(ICON_FA_SUN " Emissive", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EditorGUI::Property("Emissive Color", mat.EmissiveColor);
        EditorGUI::Property("Intensity", mat.EmissiveIntensity);
        mat.OverrideEmissive |= EditorGUI::FileProperty("Emissive Map", mat.EmissivePath, GetTextureID(mat.EmissivePath), "png,jpg,tga");
        ImGui::Checkbox("Override Emissive", &mat.OverrideEmissive);
    }

    if (ImGui::CollapsingHeader(ICON_FA_GEARS " Settings"))
    {
        ImGui::Checkbox("Double Sided", &mat.DoubleSided);
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

        std::vector<MaterialSlot>* materials = nullptr;

        if (m_SelectedEntity.HasComponent<MaterialComponent>())
            materials = &m_SelectedEntity.GetComponent<MaterialComponent>().Materials;
        else if (m_SelectedEntity.HasComponent<ModelComponent>())
            materials = &m_SelectedEntity.GetComponent<ModelComponent>().Materials;

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
            if (ImGui::Button(ICON_FA_PLUS " Add Materials Override Component"))
            {
                m_SelectedEntity.AddComponent<MaterialComponent>();
            }
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
    Panel::SetContext(context);
    m_SelectedEntity = {};
}

} // namespace CHEngine

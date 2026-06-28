#include "material_panel.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/assets/types/model_asset.h"
#include "editor/editor_gui.h"
#include "thirdparty/IconsFontAwesome6.h"

namespace Chained
{

MaterialPanel::MaterialPanel()
{
    m_Name = "Material Editor";
}

uint32_t MaterialPanel::GetTextureID(AssetHandle handle)
{
    if (handle == 0) return 0;
    auto& am = (*ServiceLocator::Get<AssetManager>());
    auto asset = am.GetAsset<TextureAsset>(handle);
    return asset ? asset->GetRendererID() : 0;
}

void MaterialPanel::DrawMaterialSettings(Material& mat)
{
    if (ImGui::CollapsingHeader(ICON_FA_IMAGE " Albedo", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Color", glm::value_ptr(mat.AlbedoColor));
        
        std::string currentPath = "";
        auto asset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(mat.AlbedoHandle);
        if (asset) currentPath = asset->GetPath();

        if (EditorGUI::FileProperty("Texture", currentPath, GetTextureID(mat.AlbedoHandle), "png,jpg,tga"))
        {
        mat.AlbedoHandle = ServiceLocator::Get<AssetManager>()->ResolveToHandle(currentPath);
            mat.AlbedoMap = 0; // Trigger reload in renderer
        }
    }

    if (ImGui::CollapsingHeader(ICON_FA_WATER " Normals", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::string normalPath = "";
        auto normalAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(mat.NormalHandle);
        if (normalAsset) normalPath = normalAsset->GetPath();

        if (EditorGUI::FileProperty("Normal Map", normalPath, GetTextureID(mat.NormalHandle), "png,jpg,tga"))
        {
            mat.NormalHandle = ServiceLocator::Get<AssetManager>()->ResolveToHandle(normalPath);
            mat.NormalMap = 0;
        }
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
        
        std::string pbrPath = "";
        auto pbrAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(mat.MetallicRoughnessHandle);
        if (pbrAsset) pbrPath = pbrAsset->GetPath();

        if (EditorGUI::FileProperty("PBR Map", pbrPath, GetTextureID(mat.MetallicRoughnessHandle), "png,jpg,tga"))
        {
            mat.MetallicRoughnessHandle = ServiceLocator::Get<AssetManager>()->ResolveToHandle(pbrPath);
            mat.MetallicRoughnessMap = 0;
        }
    }

    if (ImGui::CollapsingHeader(ICON_FA_SUN " Emissive", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Emissive Color", glm::value_ptr(mat.EmissiveColor));
        ImGui::SliderFloat("Intensity", &mat.EmissiveIntensity, 0.0f, 10.0f);
        
        std::string emissivePath = "";
        auto emissiveAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(mat.EmissiveHandle);
        if (emissiveAsset) emissivePath = emissiveAsset->GetPath();

        if (EditorGUI::FileProperty("Emissive Map", emissivePath, GetTextureID(mat.EmissiveHandle), "png,jpg,tga"))
        {
            mat.EmissiveHandle = ServiceLocator::Get<AssetManager>()->ResolveToHandle(emissivePath);
            mat.EmissiveMap = 0;
        }
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

    if (m_SelectedEntity && (!m_SelectedEntity.IsValid() || &m_SelectedEntity.GetRegistry() != &m_Context->GetRegistry()))
    {
        m_SelectedEntity = {};
    }

    if (m_SelectedEntity && m_SelectedEntity.IsValid())
    {
        ImGui::BeginDisabled(readOnly);

        if (m_SelectedEntity.HasComponent<ModelComponent>())
        {
            auto& modelComp = m_SelectedEntity.GetComponent<ModelComponent>();
            auto modelAsset = ServiceLocator::Get<AssetManager>()->GetAsset<ModelAsset>(modelComp.ModelHandle);
            
            if (modelAsset && modelAsset->IsReady())
            {
                auto& materials = modelAsset->GetMaterials();
                if (!materials.empty())
                {
                    // Material Selection Sidebar / List
                    ImGui::BeginChild("MaterialList", ImVec2(150, 0), true);
                    for (int i = 0; i < (int)materials.size(); i++)
                    {
                        std::string label = materials[i].Name;
                        if (label.empty()) label = "Material " + std::to_string(i);
                        
                        if (ImGui::Selectable(label.c_str(), m_SelectedMaterialIndex == i))
                            m_SelectedMaterialIndex = i;
                    }
                    ImGui::EndChild();
                    
                    ImGui::SameLine();
                    
                    // Material Properties
                    ImGui::BeginChild("MaterialProperties");
                    if (m_SelectedMaterialIndex < (int)materials.size())
                    {
                        ImGui::TextColored({0.2f, 0.8f, 1.0f, 1.0f}, "Editing Asset Material: %s", materials[m_SelectedMaterialIndex].Name.c_str());
                        ImGui::Separator();
                        DrawMaterialSettings(materials[m_SelectedMaterialIndex]);
                    }
                    else
                    {
                        m_SelectedMaterialIndex = 0;
                    }
                    ImGui::EndChild();
                }
                else
                {
                    ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.0f}, ICON_FA_CIRCLE_INFO " No Materials found in model asset");
                }
            }
            else
            {
                ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.0f}, ICON_FA_CIRCLE_INFO " Model asset not loaded or missing");
            }
        }
        else
        {
            ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.0f}, ICON_FA_CIRCLE_INFO " No ModelComponent found for this entity");
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
    if (m_Context.get() != context.get())
    {
        m_SelectedEntity = {};
        m_SelectedMaterialIndex = 0;
    }
    Panel::SetContext(context);
}

} // namespace Chained

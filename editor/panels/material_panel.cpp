#include "material_panel.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/components/primitive_component.h"
#include "engine/scene/components/primitive_runtime.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "property_editor.h"
#include "ui_properties.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/material_asset.h"
#include <filesystem>

namespace Chained
{

	MaterialPanel::MaterialPanel()
	{
		m_Name = "Material Editor";
	}

	static uint32_t GetTextureID(const std::shared_ptr<Texture>& tex, const std::string& path)
	{
		if (tex)
		{
			return tex->GetNativeHandle();
		}
		if (path.empty())
		{
			return 0;
		}
		auto* assetMgr = ServiceLocator::TryGet<AssetManager>();
		if (!assetMgr)
		{
			return 0;
		}
		auto texAsset = assetMgr->Get<TextureAsset>(path);
		if (texAsset && texAsset->GetTexture())
		{
			return texAsset->GetTexture()->GetNativeHandle();
		}
		return 0;
	}

	static void UpdateTextureFromPath(std::shared_ptr<Texture>& tex, const std::string& path)
	{
		if (path.empty())
		{
			tex.reset();
			return;
		}
		auto* assetMgr = ServiceLocator::TryGet<AssetManager>();
		if (!assetMgr)
		{
			return;
		}
		auto texAsset = assetMgr->Get<TextureAsset>(path);
		if (texAsset && texAsset->IsReady())
		{
			tex = texAsset->GetTexture();
		}
	}

	static bool DrawSectionHeader(const char* icon, const char* label)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, {0.2f, 0.25f, 0.35f, 0.8f});
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.3f, 0.4f, 0.6f, 1.0f});
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.25f, 0.35f, 0.5f, 1.0f});
		bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);
		ImGui::PopStyleColor(3);
		return open;
	}

	void MaterialPanel::DrawMaterialSlot(Material& mat)
	{
		if (DrawSectionHeader(ICON_FA_IMAGE, ICON_FA_IMAGE " Albedo"))
		{
			ImGui::Indent();
			EditorGUI::BeginPropertyGrid();
			EditorGUI::PropertyColor("Color", mat.AlbedoColor);
			if (EditorGUI::FileProperty("Texture", mat.AlbedoPath, GetTextureID(mat.AlbedoMap, mat.AlbedoPath),
										"png,jpg,tga"))
			{
				UpdateTextureFromPath(mat.AlbedoMap, mat.AlbedoPath);
			}
			EditorGUI::EndPropertyGrid();
			ImGui::Unindent();
		}

		if (DrawSectionHeader(ICON_FA_WATER, ICON_FA_WATER " Normals"))
		{
			ImGui::Indent();
			EditorGUI::BeginPropertyGrid();
			if (EditorGUI::FileProperty("Normal Map", mat.NormalPath, GetTextureID(mat.NormalMap, mat.NormalPath),
										"png,jpg,tga"))
			{
				UpdateTextureFromPath(mat.NormalMap, mat.NormalPath);
			}
			EditorGUI::EndPropertyGrid();
			ImGui::Unindent();
		}

		if (DrawSectionHeader(ICON_FA_CIRCLE_HALF_STROKE, ICON_FA_CIRCLE_HALF_STROKE " PBR (Metal/Rough)"))
		{
			ImGui::Indent();
			EditorGUI::BeginPropertyGrid();
			EditorGUI::Property("Metalness", mat.Metalness, 0.01f, 0.0f, 1.0f);
			EditorGUI::Property("Roughness", mat.Roughness, 0.01f, 0.0f, 1.0f);
			if (EditorGUI::FileProperty("PBR Map", mat.MetallicRoughnessPath,
										GetTextureID(mat.MetallicRoughnessMap, mat.MetallicRoughnessPath),
										"png,jpg,tga"))
			{
				UpdateTextureFromPath(mat.MetallicRoughnessMap, mat.MetallicRoughnessPath);
			}
			EditorGUI::EndPropertyGrid();
			ImGui::TextDisabled("PBR map: G = Roughness, B = Metalness (glTF convention)");
			ImGui::Unindent();
		}

		if (DrawSectionHeader(ICON_FA_SUN, ICON_FA_SUN " Emissive"))
		{
			ImGui::Indent();
			EditorGUI::BeginPropertyGrid();
			EditorGUI::PropertyColor("Color", mat.EmissiveColor, /*hdr*/ true);
			EditorGUI::Property("Intensity", mat.EmissiveIntensity, 0.05f, 0.0f, 1000.0f);
			if (EditorGUI::FileProperty("Emissive Map", mat.EmissivePath,
										GetTextureID(mat.EmissiveMap, mat.EmissivePath), "png,jpg,tga"))
			{
				UpdateTextureFromPath(mat.EmissiveMap, mat.EmissivePath);
			}
			EditorGUI::EndPropertyGrid();
			ImGui::Unindent();
		}

		if (DrawSectionHeader(ICON_FA_GEARS, ICON_FA_GEARS " Settings"))
		{
			ImGui::Indent();
			EditorGUI::BeginPropertyGrid();
			EditorGUI::Property("Transparent", mat.Transparent);
			ImGui::BeginDisabled(!mat.Transparent);
			EditorGUI::Property("Alpha", mat.Alpha, 0.01f, 0.0f, 1.0f);
			ImGui::EndDisabled();
			EditorGUI::EndPropertyGrid();
			ImGui::Unindent();
		}
	}

	void MaterialPanel::OnImGuiRender(bool readOnly)
	{
		if (!m_IsOpen)
		{
			return;
		}

		ImGui::Begin(m_Name.c_str(), &m_IsOpen);

		if (m_SelectedEntity && (!m_Context || m_SelectedEntity.GetRegistryPtr() != m_Context->GetRegistryPtr() ||
								 !m_SelectedEntity.IsValid()))
		{
			m_SelectedEntity = {};
		}

		if (m_SelectedEntity)
		{
			ImGui::BeginDisabled(readOnly);

			std::vector<Material>* materials = nullptr;

			if (m_SelectedEntity.HasComponent<ModelComponent>())
			{
				auto& mc = m_SelectedEntity.GetComponent<ModelComponent>();

				if (m_Materials.empty() && !mc.ModelPath.empty())
				{
					auto* assetMgr = ServiceLocator::TryGet<AssetManager>();
					if (assetMgr)
					{
						auto asset = assetMgr->Get<ModelAsset>(mc.ModelPath);
						if (asset && asset->IsReady())
						{
							m_Materials = asset->GetMaterials();
						}
					}
				}

				materials = &m_Materials;
			}
			// Procedural primitives own their ModelAsset inside PrimitiveRuntimeState (not serialized),
			// so edit its materials in place — the renderer reads the very same asset each frame.
			else if (m_SelectedEntity.HasComponent<PrimitiveComponent>())
			{
				auto& prim = m_SelectedEntity.GetComponent<PrimitiveComponent>();
				(void)prim; // prim fields still needed for SetMaterial below
				if (m_SelectedEntity.HasComponent<PrimitiveRuntimeState>())
				{
					auto& rt = m_SelectedEntity.GetComponent<PrimitiveRuntimeState>();
					if (rt.Asset && rt.Asset->IsReady())
					{
						materials = &rt.Asset->GetMaterials();
					}
				}
			}

			if (materials && !materials->empty())
			{
				// Material Selection Sidebar / List
				ImGui::BeginChild("MaterialList", ImVec2(180, 0), true);
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputTextWithHint("##MatFilter", ICON_FA_MAGNIFYING_GLASS " Search...", m_FilterBuffer,
										 sizeof(m_FilterBuffer));
				ImGui::Separator();

				std::string filterStr = m_FilterBuffer;
				std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

				for (int i = 0; i < (int)materials->size(); i++)
				{
					const Material& m = (*materials)[i];
					std::string label = m.Name;
					if (label.empty())
					{
						label = "Material " + std::to_string(i);
					}

					if (!filterStr.empty())
					{
						std::string lowerLabel = label;
						std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);
						if (lowerLabel.find(filterStr) == std::string::npos)
						{
							continue;
						}
					}

					ImGui::PushID(i);
					uint32_t texHandle = GetTextureID(m.AlbedoMap, m.AlbedoPath);
					if (texHandle != 0)
					{
						ImGui::Image((ImTextureID)(uintptr_t)texHandle, ImVec2(14, 14));
					}
					else
					{
						ImVec4 swatch = {m.AlbedoColor.r, m.AlbedoColor.g, m.AlbedoColor.b, 1.0f};
						ImGui::ColorButton("##swatch", swatch,
										   ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker |
											   ImGuiColorEditFlags_NoBorder,
										   {14, 14});
					}
					ImGui::SameLine();
					if (ImGui::Selectable(label.c_str(), m_SelectedMaterialIndex == i))
					{
						m_SelectedMaterialIndex = i;
					}
					ImGui::PopID();
				}
				ImGui::EndChild();

				ImGui::SameLine();

				// Material Properties — leave room for Save button
				float availH = ImGui::GetContentRegionAvail().y;
				float buttonArea = 50.0f;
				ImGui::BeginChild("MaterialProperties", ImVec2(0, availH - buttonArea));
				if (m_SelectedMaterialIndex < (int)materials->size())
				{
					Material& selected = (*materials)[m_SelectedMaterialIndex];
					std::string title =
						selected.Name.empty() ? ("Material " + std::to_string(m_SelectedMaterialIndex)) : selected.Name;
					ImGui::TextColored({0.2f, 0.8f, 1.0f, 1.0f}, ICON_FA_PALETTE " Editing: %s", title.c_str());
					ImGui::Separator();
					ImGui::Spacing();
					DrawMaterialSlot(selected);
					if (m_SelectedEntity.HasComponent<PrimitiveComponent>())
					{
						auto& prim = m_SelectedEntity.GetComponent<PrimitiveComponent>();
						prim.SetMaterial(selected);
					}
				}
				else
				{
					m_SelectedMaterialIndex = 0;
				}
				ImGui::EndChild();

				// Save button
				ImGui::Separator();
				if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Materials", ImVec2(-1, 0)))
				{
					SaveMaterials();
					ImGui::OpenPopup("Materials Saved");
				}

				if (ImGui::BeginPopup("Materials Saved"))
				{
					ImGui::Text("Materials saved successfully!");
					ImGui::EndPopup();
				}
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

	void MaterialPanel::SaveMaterials()
	{
		if (!m_SelectedEntity || !m_SelectedEntity.IsValid())
		{
			return;
		}

		if (m_SelectedEntity.HasComponent<PrimitiveComponent>())
		{
			auto& prim = m_SelectedEntity.GetComponent<PrimitiveComponent>();
			if (m_SelectedEntity.HasComponent<PrimitiveRuntimeState>())
			{
				auto& rt = m_SelectedEntity.GetComponent<PrimitiveRuntimeState>();
				if (rt.Asset && !rt.Asset->GetMaterials().empty())
				{
					prim.SetMaterial(rt.Asset->GetMaterials()[0]);
				}
			}
			return;
		}

		if (!m_SelectedEntity.HasComponent<ModelComponent>())
		{
			return;
		}
		if (m_Materials.empty())
		{
			return;
		}

		auto& mc = m_SelectedEntity.GetComponent<ModelComponent>();
		std::filesystem::path modelPath(mc.ModelPath);
		std::string modelName = modelPath.stem().string();
		std::filesystem::path modelDir = modelPath.parent_path();

		auto* assets = ServiceLocator::TryGet<AssetManager>();
		if (!assets)
		{
			return;
		}

		for (int i = 0; i < (int)m_Materials.size(); i++)
		{
			std::string matFileName = modelName + "_material_" + std::to_string(i) + ".chmat";
			std::string matPath;

			if (i < (int)mc.MaterialPaths.size() && !mc.MaterialPaths[i].empty())
			{
				matPath = mc.MaterialPaths[i];
			}
			else
			{
				matPath = (modelDir / matFileName).generic_string();
			}

			auto matAsset = std::make_shared<MaterialAsset>();
			matAsset->SetMaterial(m_Materials[i]);
			matAsset->SaveToFile(assets->ResolvePath(matPath));

			if (i >= (int)mc.MaterialPaths.size())
			{
				mc.MaterialPaths.resize(i + 1);
			}
			mc.MaterialPaths[i] = matPath;
		}

		// Write back to ModelAsset so renderer picks up changes immediately
		auto modelAsset = assets->Get<ModelAsset>(mc.ModelPath);
		if (modelAsset)
		{
			modelAsset->GetMaterials() = m_Materials;
		}
	}

	void MaterialPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<EntitySelectedEvent>([this](EntitySelectedEvent& ev) {
			SaveMaterials();
			m_SelectedEntity = Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry());
			m_SelectedMeshIndex = ev.GetMeshIndex();
			m_SelectedMaterialIndex = 0;
			m_Materials.clear();
			return false;
		});
	}

	void MaterialPanel::SetContext(const std::shared_ptr<Scene>& context)
	{
		if (m_Context != context)
		{
			SaveMaterials();
			Panel::SetContext(context);
			m_SelectedEntity = {};
			m_Materials.clear();
		}
	}

} // namespace Chained

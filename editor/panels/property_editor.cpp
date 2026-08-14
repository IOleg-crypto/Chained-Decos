#include "property_editor.h"
#include "engine/reflection/reflection_rfl.h"
#include "engine/reflection/reflection_rfl_impl.h"
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
#include "engine/scripting/scriptengine.h"
#include <Coral/ManagedObject.hpp>

#include "engine/app/application.h"
#include <yaml-cpp/yaml.h>
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/graphics/ui/ui_font_registry.h"
#include "engine/graphics/ui/widget_renderer.h"

namespace Chained
{

	// --- UI Widget Data Drawers (extracted from UIControlComponent lambda) ---

	static bool DrawButtonData(ButtonData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (ui.Property("Interactable", data.IsInteractable))
		{
			changed = true;
		}
		if (ui.Property("Auto Size", data.AutoSize))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawLabelData(LabelData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Text", data.Text))
		{
			changed = true;
		}
		if (ui.Property("Auto Size", data.AutoSize))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawCheckboxData(CheckboxData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (ui.Property("Checked", data.Checked))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawSliderData(SliderData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (ui.Property("Value", data.Value, PropertyMeta(data.Min, data.Max, 0.01f)))
		{
			changed = true;
		}
		if (ui.Property("Min", data.Min))
		{
			changed = true;
		}
		if (ui.Property("Max", data.Max))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawProgressBarData(ProgressBarData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Progress", data.Progress, PropertyMeta(0.0f, 1.0f, 0.01f)))
		{
			changed = true;
		}
		if (ui.Property("Overlay Text", data.OverlayText))
		{
			changed = true;
		}
		if (ui.Property("Show %", data.ShowPercentage))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawImageData(ImageData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.File("Texture Path", data.TexturePath, ".png,.jpg,.jpeg,.bmp,.tga"))
		{
			changed = true;
		}
		if (ui.Property("Tint Color", data.TintColor))
		{
			changed = true;
		}
		if (ui.Property("Border Color", data.BorderColor))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawPanelData(PanelData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.File("Texture Path", data.TexturePath, ".png,.jpg,.jpeg"))
		{
			changed = true;
		}
		if (ui.Property("Full Screen", data.FullScreen))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawComboBoxData(ComboBoxData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (!data.Items.empty())
		{
			if (ui.Property("Selected", data.SelectedIndex, PropertyMeta(0, (int)data.Items.size() - 1, 1)))
			{
				changed = true;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted("Items");
		ImGui::TableSetColumnIndex(1);
		int removeIdx = -1;
		for (int i = 0; i < (int)data.Items.size(); i++)
		{
			ImGui::PushID(i);
			char buf[256];
			strncpy(buf, data.Items[i].c_str(), sizeof(buf) - 1);
			buf[sizeof(buf) - 1] = 0;
			if (ImGui::InputText("##item", buf, sizeof(buf)))
			{
				data.Items[i] = buf;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton(ICON_FA_TRASH))
			{
				removeIdx = i;
				changed = true;
			}
			ImGui::PopID();
		}
		if (removeIdx >= 0)
		{
			data.Items.erase(data.Items.begin() + removeIdx);
		}
		if (ImGui::SmallButton(ICON_FA_PLUS " Add Item"))
		{
			data.Items.push_back("");
			changed = true;
		}
		return changed;
	}

	static bool DrawInputTextData(InputTextData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Text", data.Text))
		{
			data.InputBuffer.clear();
			changed = true;
		}
		if (ui.Property("Placeholder", data.Placeholder))
		{
			changed = true;
		}
		if (ui.Property("Max Length", data.MaxLength, PropertyMeta(1, 1024, 1)))
		{
			changed = true;
		}
		if (ui.Property("Multiline", data.Multiline))
		{
			changed = true;
		}
		if (ui.Property("Read Only", data.ReadOnly))
		{
			changed = true;
		}
		if (ui.Property("Password", data.Password))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawImageButtonData(ImageButtonData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.File("Texture Path", data.TexturePath, ".png,.jpg,.jpeg"))
		{
			changed = true;
		}
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawRadioButtonData(RadioButtonData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (!data.Options.empty())
		{
			if (ui.Property("Selected", data.SelectedIndex, PropertyMeta(0, (int)data.Options.size() - 1, 1)))
			{
				changed = true;
			}
		}
		if (ui.Property("Horizontal", data.Horizontal))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawDragFloatData(DragFloatData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (ui.Property("Value", data.Value, PropertyMeta(data.Min, data.Max, data.Speed)))
		{
			changed = true;
		}
		if (ui.Property("Min", data.Min))
		{
			changed = true;
		}
		if (ui.Property("Max", data.Max))
		{
			changed = true;
		}
		return changed;
	}

	static bool DrawDragIntData(DragIntData& data, UIProperties& ui)
	{
		bool changed = false;
		if (ui.Property("Label", data.Label))
		{
			changed = true;
		}
		if (ui.Property("Value", data.Value, PropertyMeta(data.Min, data.Max, 1)))
		{
			changed = true;
		}
		if (ui.Property("Min", data.Min))
		{
			changed = true;
		}
		if (ui.Property("Max", data.Max))
		{
			changed = true;
		}
		return changed;
	}

	// --- Template Implementations (Moved from Header) ---

	template <typename T>
	void PropertyEditor::DrawComponentReflection(const std::string& name, const char* icon, Entity entity)
	{
		static std::unordered_map<entt::entity, T> s_InitialStates;
		entt::entity e = (entt::entity)entity;

		// Clear stale states when entity is not in the current context
		// (handles scene changes where entity IDs may be reused)
		static entt::registry* s_LastRegistry = nullptr;
		entt::registry* currentRegistry = &entity.GetRegistry();
		if (s_LastRegistry != currentRegistry)
		{
			s_InitialStates.clear();
			s_LastRegistry = currentRegistry;
		}

		DrawComponentContainer<T>(name, icon, entity, [&](T& comp, Entity ent) {
			UIProperties ui;
			Properties props(ui);

			if constexpr (is_rfl_component<T>::value)
			{
				ReflectFromRfl(comp, props);
			}
			else
			{
				comp.Reflect(props);
			}

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

					EditorLayer::Get().GetCommandHistory().PushCommand(
						std::make_unique<ModifyComponentCommand<T>>(entity, oldState, newState, "Modify " + name));

					s_InitialStates.erase(e);
				}
			}

			return props.HasChanged();
		});
	}

	template <typename T, typename F>
	void PropertyEditor::DrawComponentContainer(const std::string& name, const char* icon, Entity entity, F&& drawer)
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
						entity.GetRegistry().template patch<T>(entity,
															   [&componentCopy](T& comp) { comp = componentCopy; });
						return true;
					}
					return false;
				},
				[&]() {
					EditorLayer::Get().GetCommandHistory().PushCommand(
						std::make_unique<RemoveComponentCommand<T>>(entity));
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
				metadata.ReflectInternal(entity, ui, ReflectionMode::UI);
				bool changed = ui.HasChanged();
				if (changed && metadata.NotifyUpdate)
				{
					// Fire registry.patch() so on_update observers (e.g. MarkPrimitiveDirty) run.
					metadata.NotifyUpdate(entity);
				}
				return changed;
			},
			[&]() {
				if (metadata.Remove)
				{
					metadata.Remove(entity);
				}
			});
	}

	template <typename T>
	void PropertyEditor::RegisterComponentImpl(const std::string& name, const char* icon,
											   std::function<void(Entity)> drawUI)
	{
		auto typeId = entt::type_hash<T>::value();

		// Register fresh metadata if the component type doesn't exist yet
		if (!ComponentRegistry::Exists(typeId))
		{
			ComponentMetadata fresh;
			fresh.Name = name;
			fresh.Icon = icon;
			fresh.Category = "Engine";
			fresh.SerializationKey = name + "Component";
			ComponentRegistry::Register(typeId, fresh);
		}

		// Apply editor-specific overrides (undo/redo, custom DrawUI)
		ComponentMetadata override;
		override.Name = name;
		override.Icon = icon;
		override.DrawUI = drawUI;
		override.Add = [](Entity e) {
			if (!e.HasComponent<T>())
			{
				EditorLayer::Get().GetCommandHistory().PushCommand(std::make_unique<AddComponentCommand<T>>(e));
			}
		};
		override.Remove = [](Entity e) {
			EditorLayer::Get().GetCommandHistory().PushCommand(std::make_unique<RemoveComponentCommand<T>>(e));
		};
		ComponentRegistry::OverrideMetadata(typeId, override);
	}

	template <typename T> void PropertyEditor::Register(const std::string& name, const char* icon)
	{
		RegisterComponentImpl<T>(name, icon, [name, icon](Entity e) { DrawComponentReflection<T>(name, icon, e); });
	}

	template <typename T, typename F>
	void PropertyEditor::RegisterCustom(const std::string& name, F&& drawer, const char* icon)
	{
		RegisterComponentImpl<T>(name, icon, [name, icon, drawer = std::forward<F>(drawer)](Entity e) {
			DrawComponentContainer<T>(name, icon, e, drawer);
		});
	}

	// --- Implementation ---

	void PropertyEditor::Init()
	{
		// --- Core Components ---
		ComponentRegistry::SetAllowAdd(entt::type_hash<TransformComponent>::value(), false);
		RegisterCustom<LightComponent>(
			"Light",
			[&](LightComponent& comp, Entity entity) {
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
				if (ui.Property("Color", comp.LightColor))
				{
					changed = true;
				}
				if (ui.Property("Intensity", comp.Intensity, PropertyMeta(0.0f, 10000.0f, 5.0f)))
				{
					changed = true;
				}
				if (ui.Property("Range", comp.Radius, PropertyMeta(0.0f, 1000.0f, 1.0f)))
				{
					changed = true;
				}

				if (comp.Type == LightType::Spot)
				{
					if (ui.Property("Inner Cutoff", comp.InnerCutoff, PropertyMeta(0.0f, 90.0f, 0.5f)))
					{
						changed = true;
					}
					if (ui.Property("Outer Cutoff", comp.OuterCutoff, PropertyMeta(0.0f, 90.0f, 0.5f)))
					{
						changed = true;
					}
				}

				if (ui.Property("Cast Shadows", comp.Shadows))
				{
					changed = true;
				}

				return changed;
			},
			ICON_FA_LIGHTBULB);

		RegisterCustom<ColliderComponent>(
			"Collider",
			[&](ColliderComponent& comp, Entity entity) {
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
					if (ui.Property("Size", comp.Size, PropertyMeta(0.01f, 100.0f, 0.05f)))
					{
						changed = true;
					}
				}
				else if (comp.Type == ColliderType::Sphere || comp.Type == ColliderType::Capsule)
				{
					if (ui.Property("Radius", comp.Radius, PropertyMeta(0.0f, 500.0f, 0.05f)))
					{
						changed = true;
					}
				}
				if (comp.Type == ColliderType::Capsule)
				{
					if (ui.Property("Height", comp.Height, PropertyMeta(0.0f, 500.0f, 0.05f)))
					{
						changed = true;
					}
				}

				if (ui.Property("Offset", comp.Offset, PropertyMeta(-10.0f, 10.0f, 0.05f)))
				{
					changed = true;
				}

				if (comp.Type == ColliderType::Mesh)
				{
					if (ui.Property("Auto Calculate", comp.AutoCalculate))
					{
						changed = true;
					}
					if (!comp.AutoCalculate)
					{
						if (ui.File("Model Path", comp.ModelPath, ".glb,.gltf,.obj"))
						{
							changed = true;
						}
					}
				}

				if (ui.Property("Friction", comp.Friction, PropertyMeta(0.0f, 1.0f, 0.01f)))
				{
					changed = true;
				}
				if (ui.Property("Restitution", comp.Restitution, PropertyMeta(0.0f, 1.0f, 0.01f)))
				{
					changed = true;
				}
				if (ui.Property("Is Trigger", comp.IsTrigger))
				{
					changed = true;
				}
				if (ui.Property("Enabled", comp.Enabled))
				{
					changed = true;
				}

				return changed;
			},
			ICON_FA_SHIELD);

		// --- Scripting ---
		RegisterCustom<ManagedScriptComponent>(
			"Scripts",
			[](ManagedScriptComponent& comp, Entity entity) {
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
					std::string shortName =
						(lastDot == std::string::npos) ? fullClassName : fullClassName.substr(lastDot + 1);
					std::string label = shortName.empty() ? "-- Empty Script --" : shortName;

					float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
					bool open =
						ImGui::TreeNodeEx((void*)(uintptr_t)i, flags, "%s %s", ICON_FA_FILE_CODE, label.c_str());

					// Tooltip with full name
					if (ImGui::IsItemHovered() && !fullClassName.empty())
					{
						ImGui::SetTooltip("%s", fullClassName.c_str());
					}

					// Delete button in the header row (right aligned in column 1)
					ImGui::TableSetColumnIndex(1);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - lineHeight - 5.0f);
					if (ImGui::Button(ICON_FA_TRASH, ImVec2{lineHeight, lineHeight}))
					{
						comp.Scripts.erase(comp.Scripts.begin() + i);
						changed = true;
						if (open)
						{
							ImGui::TreePop();
						}
						ImGui::PopID();
						break;
					}

					if (open)
					{
						UIProperties ui;
						// Manually draw fields from the map, skipping redundancy
						for (auto& [fieldName, field] : script.Fields)
						{
							std::visit(
								[&](auto&& val) {
									if (ui.Property(fieldName.c_str(), val))
									{
										changed = true;
									}
								},
								field.Value);
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
					if (auto* se = ServiceLocator::TryGet<ScriptEngine>())
					{
						for (const auto& [className, type] : se->GetRegistry().GetScriptClasses())
						{
							// Extract short name for menu
							size_t lastDot = className.find_last_of('.');
							std::string shortName =
								(lastDot == std::string::npos) ? className : className.substr(lastDot + 1);

							if (ImGui::MenuItem(shortName.c_str()))
							{
								comp.Scripts.emplace_back(className);
								changed = true;
							}
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("%s", className.c_str());
							}
						}
					}
					ImGui::EndPopup();
				}

				return changed;
			},
			ICON_FA_FILE_CODE);

		RegisterCustom<ModelComponent>(
			"Model",
			[&](ModelComponent& comp, Entity entity) {
				bool changed = false;
				UIProperties ui;
				Properties props(ui);

				if (ui.File("Model Path", comp.ModelPath, ".glb,.gltf,.obj"))
				{
					comp.ModelHandle = AssetHandle(0);
					comp.MaterialPaths.clear();
					changed = true;
				}

				auto* am = ServiceLocator::TryGet<AssetManager>();
				if (am && !comp.ModelPath.empty())
				{
					auto handle = am->ResolveToHandle(comp.ModelPath);
					if (handle != AssetHandle(0))
					{
						auto asset = am->Get<ModelAsset>(handle);
						if (asset)
						{
							const char* stateStr = "Unknown";
							ImVec4 stateColor(0.7f, 0.7f, 0.7f, 1.0f);
							switch (asset->GetState())
							{
							case AssetState::Ready:
								stateStr = "Ready";
								stateColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
								break;
							case AssetState::Loading:
								stateStr = "Loading";
								stateColor = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
								break;
							case AssetState::Failed:
								stateStr = "Failed";
								stateColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
								break;
							default:
								break;
							}
							ImGui::SameLine();
							ImGui::TextColored(stateColor, "%s", stateStr);

							if (asset->GetState() == AssetState::Ready)
							{
								ImGui::SameLine();
								if (ImGui::SmallButton("Reload"))
								{
									am->Invalidate(comp.ModelPath);
									comp.ModelHandle = AssetHandle(0);
									comp.MaterialPaths.clear();
									CH_CORE_INFO("ModelComponent: Invalidated '{}', will reload next frame",
												 comp.ModelPath);
								}
								ImGui::SameLine();
								if (ImGui::SmallButton("Delete .chasset"))
								{
									am->DeleteChasset(comp.ModelPath);
									am->Invalidate(comp.ModelPath);
									comp.ModelHandle = AssetHandle(0);
									comp.MaterialPaths.clear();
									CH_CORE_INFO("ModelComponent: Deleted .chasset for '{}', will re-import next frame",
												 comp.ModelPath);
								}
							}
						}
					}
				}

				return changed;
			},
			ICON_FA_SHAPES);

		// --- UI Components ---

		// --- UI Widgets ---
		RegisterCustom<UIControlComponent>(
			"Widget",
			[](UIControlComponent& comp, Entity entity) {
				bool changed = false;
				UIProperties ui;

				// Box Style
				ui.Header("Box Style");
				if (ui.Property("BG Color", comp.BoxStyle.BackgroundColor))
				{
					changed = true;
				}
				if (ui.Property("Hover Color", comp.BoxStyle.HoverColor))
				{
					changed = true;
				}
				if (ui.Property("Pressed Color", comp.BoxStyle.PressedColor))
				{
					changed = true;
				}
				if (ui.Property("Border Color", comp.BoxStyle.BorderColor))
				{
					changed = true;
				}
				if (ui.Property("Rounding", comp.BoxStyle.Rounding, PropertyMeta(0.0f, 32.0f, 0.5f)))
				{
					changed = true;
				}
				if (ui.Property("Border Size", comp.BoxStyle.BorderSize, PropertyMeta(0.0f, 10.0f, 0.1f)))
				{
					changed = true;
				}
				if (ui.Property("Padding", comp.BoxStyle.Padding, PropertyMeta(0.0f, 64.0f, 0.5f)))
				{
					changed = true;
				}
				if (ui.Property("Hover Scale", comp.BoxStyle.HoverScale, PropertyMeta(0.5f, 3.0f, 0.01f)))
				{
					changed = true;
				}
				if (ui.Property("Pressed Scale", comp.BoxStyle.PressedScale, PropertyMeta(0.5f, 3.0f, 0.01f)))
				{
					changed = true;
				}
				if (ui.Property("Transition Speed", comp.BoxStyle.TransitionSpeed, PropertyMeta(0.0f, 2.0f, 0.01f)))
				{
					changed = true;
				}
				if (ui.Property("Gradient", comp.BoxStyle.UseGradient))
				{
					changed = true;
				}
				if (ui.Property("Gradient Color", comp.BoxStyle.GradientColor))
				{
					changed = true;
				}

				ui.Separator();
				// Text Style
				ui.Header("Text Style");
				{
					auto* fontRegistry = ServiceLocator::TryGet<UIFontRegistry>();
					auto fontNames = fontRegistry ? fontRegistry->GetKnownFontNames() : std::vector<std::string>{};
					fontNames.insert(fontNames.begin(), "Default");
					if (ui.StringEnum("Font Name", comp.TextStyle.FontName, fontNames))
					{
						changed = true;
					}
				}
				if (ui.Property("Font Size", comp.TextStyle.FontSize, PropertyMeta(4.0f, 256.0f, 0.5f)))
				{
					changed = true;
				}
				if (ui.Property("Text Color", comp.TextStyle.TextColor))
				{
					changed = true;
				}
				if (ui.Property("Shadow", comp.TextStyle.Shadow))
				{
					changed = true;
				}
				if (comp.TextStyle.Shadow)
				{
					if (ui.Property("Shadow Offset", comp.TextStyle.ShadowOffset, PropertyMeta(0.0f, 20.0f, 0.5f)))
					{
						changed = true;
					}
					if (ui.Property("Shadow Color", comp.TextStyle.ShadowColor))
					{
						changed = true;
					}
				}
				if (ui.Property("Letter Spacing", comp.TextStyle.LetterSpacing, PropertyMeta(0.0f, 10.0f, 0.05f)))
				{
					changed = true;
				}
				if (ui.Property("Line Height", comp.TextStyle.LineHeight, PropertyMeta(0.0f, 5.0f, 0.05f)))
				{
					changed = true;
				}
				if (ui.Property("H Align", comp.TextStyle.Horizontal))
				{
					changed = true;
				}
				if (ui.Property("V Align", comp.TextStyle.Vertical))
				{
					changed = true;
				}

				ui.Separator();
				// Widget-type specific
				std::visit(
					[&](auto&& data) {
						using T = std::decay_t<decltype(data)>;
						if constexpr (std::is_same_v<T, ButtonData>)
						{
							changed = DrawButtonData(data, ui);
						}
						else if constexpr (std::is_same_v<T, LabelData>)
						{
							changed = DrawLabelData(data, ui);
						}
						else if constexpr (std::is_same_v<T, CheckboxData>)
						{
							changed = DrawCheckboxData(data, ui);
						}
						else if constexpr (std::is_same_v<T, SliderData>)
						{
							changed = DrawSliderData(data, ui);
						}
						else if constexpr (std::is_same_v<T, ProgressBarData>)
						{
							changed = DrawProgressBarData(data, ui);
						}
						else if constexpr (std::is_same_v<T, ImageData>)
						{
							changed = DrawImageData(data, ui);
						}
						else if constexpr (std::is_same_v<T, PanelData>)
						{
							changed = DrawPanelData(data, ui);
						}
						else if constexpr (std::is_same_v<T, ComboBoxData>)
						{
							changed = DrawComboBoxData(data, ui);
						}
						else if constexpr (std::is_same_v<T, InputTextData>)
						{
							changed = DrawInputTextData(data, ui);
						}
						else if constexpr (std::is_same_v<T, ImageButtonData>)
						{
							changed = DrawImageButtonData(data, ui);
						}
						else if constexpr (std::is_same_v<T, RadioButtonData>)
						{
							changed = DrawRadioButtonData(data, ui);
						}
						else if constexpr (std::is_same_v<T, DragFloatData>)
						{
							changed = DrawDragFloatData(data, ui);
						}
						else if constexpr (std::is_same_v<T, DragIntData>)
						{
							changed = DrawDragIntData(data, ui);
						}
					},
					comp.Data);

				return changed;
			},
			ICON_FA_SHAPES);

		// Mark only real UI widget types as IsWidget (these will be hidden in 3D scenes)
		auto markWidget = [&](entt::id_type id) { ComponentRegistry::SetIsWidget(id, true); };
		markWidget(entt::type_hash<ControlComponent>::value());
		markWidget(entt::type_hash<UIActionComponent>::value());
		markWidget(entt::type_hash<UIControlComponent>::value());
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
				{
					metadata.DrawUI(entity);
				}
				else if (metadata.IsReflective && metadata.ReflectInternal)
				{
					DrawGenericReflection(metadata, entity);
				}
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

	void PropertyEditor::DrawAddComponentPopup(Entity entity)
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

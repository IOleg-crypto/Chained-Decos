#include "editor_menu.h"
#include "editor/editor_colors.h"
#include "editor/layer.h"
#include "editor/panels.h"
#include "editor/project/project_exporter.h"
#include "engine/app/application.h"
#include "engine/common/thread_pool.h"
#include "engine/core/service_locator.h"
#include "engine/platform/dialogs/dialogs.h"
#include "engine/project/project.h"
#include "events.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "scripting/scriptengine.h"
#include "editor/scene_manager.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "engine/assets/asset_manager.h"
#include "editor/font_choice_gui.h"

#include <filesystem>

namespace Chained
{

	void EditorMenu::DrawMenuBar(EditorPanels& panels)
	{
		if (!ImGui::BeginMenuBar())
		{
			return;
		}

		// File Menu
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem(ICON_FA_FILE " New Project", "Ctrl+Shift+N"))
			{
				auto newScene = Scene::CreateDefault();
				EditorLayer::Get().GetSceneManager().SetScene(newScene);
			}
			if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project", "Ctrl+O"))
			{
				std::vector<DialogFilter> filters = {{"Chained Scene", "chscene"}};
				auto result = Chained::Dialogs::OpenFile(filters);
				if (result)
				{
					EditorLayer::Get().GetSceneManager().OpenScene(*result);
				}
			}
			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project"))
			{
				EditorLayer::Get().GetSceneManager().SaveScene();
			}
			if (ImGui::MenuItem(ICON_FA_XMARK " Close Project"))
			{
				Project::SetActive(nullptr);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_FILE_CODE " New Scene", "Ctrl+N"))
			{
				EditorLayer::Get().GetSceneManager().NewScene();
			}
			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene", "Ctrl+S"))
			{
				EditorLayer::Get().GetSceneManager().SaveScene();
			}
			if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Scene As...", "Ctrl+Shift+S"))
			{
				EditorLayer::Get().GetSceneManager().SaveSceneAs();
			}
			if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load Scene", "Ctrl+L"))
			{
				EditorLayer::Get().GetSceneManager().OpenScene();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_POWER_OFF " Exit"))
			{
				Application::Get().Close();
			}
			ImGui::EndMenu();
		}

		// View Menu
		if (ImGui::BeginMenu("View"))
		{
			panels.ForEach([](const std::shared_ptr<Panel>& panel) {
				if (panel->GetName() != "Viewport" && panel->GetName() != "Project Browser")
				{
					ImGui::MenuItem(panel->GetName().c_str(), nullptr, &panel->IsOpen());
				}
			});
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_EXPAND " Fullscreen", "F11"))
			{
				Application::Get().GetWindow().ToggleFullscreen();
			}
			if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reset Layout"))
			{
				AppResetLayoutEvent e;
				Application::Get().OnEvent(e);
			}
			if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Layout"))
			{
				AppSaveLayoutEvent e;
				Application::Get().OnEvent(e);
			}
			ImGui::EndMenu();
		}

		// Project Menu
		if (ImGui::BeginMenu("Project"))
		{
			if (ImGui::MenuItem(ICON_FA_GEARS " Settings"))
			{
				if (auto p = panels.Get("Project Settings"))
				{
					p->IsOpen() = true;
				}
			}
			bool isExporting = false;
			{
				std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
				isExporting = m_ExportState.IsExporting;
			}

			if (ImGui::MenuItem(isExporting ? ICON_FA_FILE_EXPORT " Exporting..."
											: ICON_FA_FILE_EXPORT " Export Project..."))
			{
				if (!isExporting)
				{
					m_ExportDialog.Open = true;
					auto project = Project::GetActive();
					if (project)
					{
						m_ExportDialog.SelectedMode = project->GetConfig().Export.Mode;
						m_ExportDialog.ZipThreshold = project->GetConfig().Export.ZipThreshold;
						m_ExportDialog.DataVersion = project->GetConfig().Export.DataVersion;
					}
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reload Shaders"))
			{
				if (auto* renderer = ServiceLocator::TryGet<Renderer>())
				{
					renderer->GetShaderLibrary().ReloadAll();
				}
			}
			if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reload All Stale Assets"))
			{
				auto* am = ServiceLocator::TryGet<AssetManager>();
				if (am)
				{
					size_t count = am->ReloadAllStale();
					CH_CORE_INFO("EditorMenu: Reloaded {} stale assets", count);
				}
			}
			if (ImGui::MenuItem(ICON_FA_TRASH " Clear .chasset Cache"))
			{
				auto* am = ServiceLocator::TryGet<AssetManager>();
				if (am)
				{
					size_t count = am->DeleteAllChassets();
					CH_CORE_INFO("EditorMenu: Deleted {} .chasset file(s)", count);
				}
			}
			if (ImGui::MenuItem(ICON_FA_FILE_CODE " Reload Scripts", "Ctrl+R"))
			{
				auto project = Project::GetActive();
				if (project)
				{
					auto assemblyPath = ScriptEngine::ResolveAssemblyPath(project->GetConfig().Scripting,
																		  project->GetConfig().ProjectDirectory);
					if (auto* scriptEngine = ServiceLocator::TryGet<ScriptEngine>())
					{
						scriptEngine->RequestAssemblyReload(assemblyPath.string(), "EditorGUI");
					}
				}
			}
			ImGui::EndMenu();
		}

		// Editor Menu
		if (ImGui::BeginMenu("Editor"))
		{
			if (ImGui::MenuItem(ICON_FA_SLIDERS " Settings"))
			{
				m_ShowEditorSettings = true;
			}
			ImGui::EndMenu();
		}

		// ── Main Menu Bar Playback Controls (Play / Simulate) ────────────────────
		float barWidth = ImGui::GetWindowWidth();
		float centerPos = (barWidth - 160.0f) * 0.5f;
		if (centerPos > ImGui::GetCursorPosX())
		{
			ImGui::SameLine(centerPos);
		}

		SceneState sceneState = EditorLayer::Get().GetSceneManager().GetSceneState();
		bool isPlaying = (sceneState == SceneState::Play);
		bool isSimulating = (sceneState == SceneState::Simulate);

		// Play / Stop Button
		if (isPlaying)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::PlayGreen);
		}
		if (ImGui::Button(isPlaying ? (ICON_FA_STOP " Stop") : (ICON_FA_PLAY " Play"), ImVec2(65, 20)))
		{
			EditorLayer::Get().GetSceneManager().SetSceneState(isPlaying ? SceneState::Edit : SceneState::Play);
		}
		if (isPlaying)
		{
			ImGui::PopStyleColor();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isPlaying ? "Stop Game" : "Play Game (Run Physics & Scripts)");
		}

		ImGui::SameLine(0, 5);

		// Simulate / Stop Button
		if (isSimulating)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::SimulateOrange);
		}
		if (ImGui::Button(isSimulating ? (ICON_FA_STOP " Stop") : (ICON_FA_GEARS " Simulate"), ImVec2(80, 20)))
		{
			EditorLayer::Get().GetSceneManager().SetSceneState(isSimulating ? SceneState::Edit : SceneState::Simulate);
		}
		if (isSimulating)
		{
			ImGui::PopStyleColor();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(isSimulating ? "Stop Simulation" : "Simulate (Physics Only)");
		}

		// ── Export progress: snapshot state here, rendering happens in DrawExportProgressOverlay() ──
		// (ImGui::Begin cannot be called inside BeginMenuBar/EndMenuBar context.)
		// ── Export result popup ───────────────────────────────────────────────────
		// Snapshot mutable state under the lock, then render without holding it.
		// NOTE: ImGui::OpenPopup must NOT be called while holding m_ExportState.Mutex:
		// ImGui is not thread-safe and doing so could deadlock if a background thread
		// also tries to acquire the mutex while ImGui is in the middle of state mutation.
		bool shouldOpenExportPopup = false;
		{
			std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
			if (m_ExportState.Open)
			{
				m_ExportResultSuccess = m_ExportState.Success;
				m_ExportResultMessage = m_ExportState.Message;
				m_ExportResultOutDir = m_ExportState.OutDir;
				m_ExportState.Open = false;
				shouldOpenExportPopup = true;
			}
		}
		if (shouldOpenExportPopup)
		{
			ImGui::OpenPopup("Export Result");
		}
		if (ImGui::BeginPopupModal("Export Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (m_ExportResultSuccess)
			{
				ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), ICON_FA_CIRCLE_INFO " Success");
			}
			else
			{
				ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Failed");
			}
			ImGui::Spacing();
			ImGui::TextWrapped("%s", m_ExportResultMessage.c_str());
			if (!m_ExportResultOutDir.empty())
			{
				ImGui::Spacing();
				ImGui::Text("Output: ");
				ImGui::SameLine();
				ImGui::TextDisabled("%s", m_ExportResultOutDir.c_str());
			}
			ImGui::Spacing();
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120.f, 0.f)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// --- Unsaved Changes Confirm Dialog ---
		{
			auto& sceneMgr = EditorLayer::Get().GetSceneManager();
			if (sceneMgr.IsConfirmPending())
			{
				ImGui::OpenPopup("Unsaved Changes");
			}
			if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Scene has unsaved changes.");
				ImGui::Spacing();
				ImGui::TextDisabled("Do you want to save before continuing?");
				ImGui::Spacing();
				ImGui::Separator();

				if (ImGui::Button("Save", ImVec2(120.f, 0.f)))
				{
					sceneMgr.SaveScene();
					sceneMgr.ConfirmPendingAction();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Don't Save", ImVec2(120.f, 0.f)))
				{
					sceneMgr.ConfirmPendingAction();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
				{
					sceneMgr.CancelPendingAction();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		ImGui::EndMenuBar();
	}

	void EditorMenu::DrawExportDialog()
	{
		if (!m_ExportDialog.Open)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Once);
		if (ImGui::Begin("Export Project", &m_ExportDialog.Open))
		{
			ImGui::Text("Choose export mode:");
			ImGui::Spacing();

			struct ModeInfo
			{
				PackMode mode;
				const char* label;
				const char* desc;
			};
			ModeInfo modes[] = {
				{PackMode::Fast, "Fast", "LZ4 HC compression. Faster export, larger pack file."},
				{PackMode::Balanced, "Balanced", "ZSTD compression. Slower export, smaller pack file."},
				{PackMode::Raw, "Raw", "No compression. Stored as-is."},
			};

			for (const auto& m : modes)
			{
				bool selected = (m_ExportDialog.SelectedMode == m.mode);
				if (ImGui::RadioButton(m.label, selected))
				{
					m_ExportDialog.SelectedMode = m.mode;
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%s", m.desc);
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::SliderFloat("Compression Threshold", &m_ExportDialog.ZipThreshold, 0.0f, 1.0f, "%.2f");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Files with compression ratio above this threshold stay uncompressed.\n0.0 = "
								  "compress everything, 1.0 = compress nothing.");
			}

			{
				int dataVersion = static_cast<int>(m_ExportDialog.DataVersion);
				if (ImGui::InputInt("Data Version", &dataVersion))
				{
					m_ExportDialog.DataVersion = static_cast<uint32_t>(dataVersion);
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Increment to invalidate cached packs at runtime.");
				}
			}

			ImGui::Spacing();
			ImGui::Checkbox("Force repack", &m_ExportDialog.ForceRepack);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Rebuild resources.pack even when it is already up to date.\nBy default the pack is "
								  "reused if no source file changed.");
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("Browse Output Folder...", ImVec2(-1, 0)))
			{
				auto outDir = Dialogs::PickFolder();
				if (outDir)
				{
					m_ExportDialog.OutputDir = outDir->string();

					// Save export settings to project config
					auto project = Project::GetActive();
					if (project)
					{
						project->GetConfig().Export.Mode = m_ExportDialog.SelectedMode;
						project->GetConfig().Export.ZipThreshold = m_ExportDialog.ZipThreshold;
						project->GetConfig().Export.DataVersion = m_ExportDialog.DataVersion;
					}

					m_ExportDialog.Open = false;

					// Start the export
					{
						std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
						m_ExportState.IsExporting = true;
						m_ExportState.PackedFiles = 0;
						m_ExportState.TotalFiles = 0;
						m_ExportState.CurrentFile.clear();
					}
					m_ExportState.CancelRequested.store(false, std::memory_order_relaxed);

					auto* threadPool = ServiceLocator::TryGet<ThreadPool>();
					if (!threadPool)
					{
						CH_CORE_ERROR("EditorMenu: ThreadPool not available, cannot export");
						std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
						m_ExportState.IsExporting = false;
					}
					else
					{
						std::string outDirPath = m_ExportDialog.OutputDir;
						bool forceRepack = m_ExportDialog.ForceRepack;
						threadPool->QueueTask([outDirPath, forceRepack, this]() {
							ExportProgressCallback progressCb = [this](uint64_t packed, uint64_t total,
																	   const std::string& file) {
								std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
								m_ExportState.PackedFiles = packed;
								m_ExportState.TotalFiles = total;
								m_ExportState.CurrentFile = file;
							};
							auto result = ProjectExporter::ExportTo(outDirPath, progressCb,
																	&m_ExportState.CancelRequested, forceRepack);
							std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
							m_ExportState.Success = result.Success;
							m_ExportState.Message = result.Cancelled  ? "Export cancelled."
													: !result.Success ? ("Export failed: " + result.Error)
													: result.PackSkipped
														? "Export complete! (pack reused — no asset changes)"
														: "Export complete!";
							m_ExportState.OutDir = result.Cancelled ? "" : result.OutDir.string();
							m_ExportState.Open = true;
							m_ExportState.IsExporting = false;
						});
					}
				}
			}

			ImGui::End();
		}
	}

	void EditorMenu::DrawExportProgressOverlay()
	{
		bool showProgress = false;
		uint64_t packed = 0, total = 0;
		std::string currentFile;
		{
			std::lock_guard<std::mutex> lock(m_ExportState.Mutex);
			showProgress = m_ExportState.IsExporting;
			packed = m_ExportState.PackedFiles;
			total = m_ExportState.TotalFiles;
			currentFile = m_ExportState.CurrentFile;
		}

		if (!showProgress)
		{
			return;
		}

		// Position: bottom-right corner with a small margin.
		ImGuiViewport* vp = ImGui::GetMainViewport();
		const float margin = 16.0f;
		const float windowW = 400.0f;
		ImVec2 winPos =
			ImVec2(vp->WorkPos.x + vp->WorkSize.x - windowW - margin, vp->WorkPos.y + vp->WorkSize.y - margin);
		ImGui::SetNextWindowPos(winPos, ImGuiCond_Always, ImVec2(0.0f, 1.0f));
		ImGui::SetNextWindowSize(ImVec2(windowW, 0.0f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.92f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));

		if (ImGui::Begin("##ExportProgress", nullptr,
						 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
							 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize))
		{
			// ── Title
			ImGui::TextColored(ImVec4(0.55f, 0.85f, 1.0f, 1.0f), ICON_FA_FILE_EXPORT "  Exporting Project");
			ImGui::Spacing();

			// ── File counter
			if (total > 0)
			{
				ImGui::Text("Packed %llu of %llu files", (unsigned long long)packed, (unsigned long long)total);
			}
			else
			{
				ImGui::TextDisabled("Preparing...");
			}

			ImGui::Spacing();

			// ── Progress bar
			float fraction = (total > 0) ? static_cast<float>(packed) / static_cast<float>(total) : 0.0f;
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.25f, 0.65f, 1.0f, 1.0f));
			ImGui::ProgressBar(fraction, ImVec2(-1.0f, 8.0f), "");
			ImGui::PopStyleColor();

			// ── Current file hint
			if (!currentFile.empty())
			{
				ImGui::Spacing();
				ImGui::TextDisabled("%s", currentFile.c_str());
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ── Cancel button
			bool alreadyCancelling = m_ExportState.CancelRequested.load(std::memory_order_relaxed);
			if (alreadyCancelling)
			{
				ImGui::BeginDisabled();
			}

			if (ImGui::Button(alreadyCancelling ? ICON_FA_BOLT " Cancelling..." : ICON_FA_BOLT " Cancel",
							  ImVec2(-1.0f, 0.0f)))
			{
				m_ExportState.CancelRequested.store(true, std::memory_order_relaxed);
			}

			if (alreadyCancelling)
			{
				ImGui::EndDisabled();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	void EditorMenu::DrawEditorSettings()
	{

		if (!m_ShowEditorSettings)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(700, 480), ImGuiCond_FirstUseEver);
		auto& config = EditorLayer::Get().GetConfig();

		if (ImGui::Begin(ICON_FA_SLIDERS " Editor Settings", &m_ShowEditorSettings))
		{
			static int selectedCategory = 0;
			const char* categories[] = {ICON_FA_PALETTE " Appearance",	  ICON_FA_CAMERA " Camera",
										ICON_FA_VIDEO " Viewport",		  ICON_FA_IMAGE " Content Browser",
										ICON_FA_FLOPPY_DISK " Auto-Save", ICON_FA_ROCKET " Startup",
										ICON_FA_GEAR " General"};

			float buttonRowHeight = ImGui::GetFrameHeightWithSpacing();

			// --- Left sidebar ---
			ImGui::BeginChild("EditorSettingsSidebar", ImVec2(180, -buttonRowHeight), ImGuiChildFlags_NavFlattened);
			for (int i = 0; i < IM_ARRAYSIZE(categories); i++)
			{
				if (ImGui::Selectable(categories[i], selectedCategory == i))
				{
					selectedCategory = i;
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// --- Right content ---
			ImGui::BeginChild("EditorSettingsContent", ImVec2(0, -buttonRowHeight), ImGuiChildFlags_NavFlattened);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

			if (selectedCategory == 0) // Appearance
			{
				ImGui::TextDisabled("Font");
				ImGui::Separator();
				ImGui::Spacing();

				const auto& fontChoices = GetEditorFontChoices();
				int currentFont = -1;
				for (int i = 0; i < (int)fontChoices.size(); i++)
				{
					if (config.FontPath == fontChoices[i].Path)
					{
						currentFont = i;
						break;
					}
				}
				const char* preview = currentFont >= 0 ? fontChoices[currentFont].Label.c_str() : "Custom";
				if (ImGui::BeginCombo("Editor Font", preview))
				{
					for (int i = 0; i < (int)fontChoices.size(); i++)
					{
						bool sel = (currentFont == i);
						if (ImGui::Selectable(fontChoices[i].Label.c_str(), sel))
						{
							config.FontPath = fontChoices[i].Path;
						}
						if (sel)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::DragFloat("Font Size", &config.FontSize, 0.25f, 8.0f, 48.0f, "%.0f px");

				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::TextDisabled("Viewport Icons");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::DragFloat("Icon Scale", &config.IconSizeScale, 0.005f, 0.01f, 1.0f, "%.3f");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("How fast gizmo icons grow with camera distance.");
				}
				ImGui::DragFloat("Icon Min Size", &config.IconSizeMin, 0.05f, 0.1f, config.IconSizeMax, "%.2f");
				ImGui::DragFloat("Icon Max Size", &config.IconSizeMax, 0.05f, config.IconSizeMin, 40.0f, "%.2f");
			}
			else if (selectedCategory == 1) // Camera
			{
				ImGui::TextDisabled("Editor Camera (Edit Mode)");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::SliderFloat("Move Speed", &config.CameraMoveSpeed, 0.1f, 100.0f, "%.1f");
				ImGui::SliderFloat("Boost Multiplier", &config.CameraBoostMultiplier, 1.0f, 10.0f, "%.1f");
				ImGui::SliderFloat("Rotation Speed", &config.CameraRotationSpeed, 0.1f, 5.0f, "%.1f");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("How fast the camera rotates when holding right-click.");
				}
				ImGui::SliderFloat("Zoom Speed", &config.CameraZoomSpeedMultiplier, 0.1f, 5.0f, "%.1f");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Multiplier for mouse wheel zoom speed.");
				}
				ImGui::DragFloat("FOV", &config.CameraFovDegrees, 0.5f, 20.0f, 120.0f, "%.1f deg");
				ImGui::DragFloat("Near Clip", &config.CameraNearClip, 0.01f, 0.001f, 10.0f, "%.3f");
				ImGui::DragFloat("Far Clip", &config.CameraFarClip, 100.0f, 100.0f, 100000.0f, "%.0f");
				ImGui::Checkbox("Disable Camera Zoom", &config.DisableCameraZoom);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Prevent the mouse wheel from zooming the editor camera.");
				}
			}
			else if (selectedCategory == 2) // Viewport
			{
				ImGui::TextDisabled("Viewport");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Show Editor Icons", &config.ShowEditorIcons);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Show camera, light, and spawn zone icons in the viewport.");
				}
				ImGui::DragFloat("Gizmo Scale", &config.GizmoScale, 0.05f, 0.5f, 3.0f, "%.2f");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Scale of the transform gizmo in the viewport.");
				}
			}
			else if (selectedCategory == 3) // Content Browser
			{
				ImGui::TextDisabled("Content Browser");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::DragFloat("Thumbnail Size", &config.DefaultThumbnailSize, 4.0f, 32.0f, 256.0f, "%.0f px");
				const char* sortNames[] = {"Name", "Date", "Size"};
				ImGui::Combo("Sort Order", &config.DefaultSortOrder, sortNames, 3);
				ImGui::Checkbox("Show File Extensions", &config.ShowFileExtensions);
			}
			else if (selectedCategory == 4) // Auto-Save
			{
				ImGui::TextDisabled("Auto-Save");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Enable Auto-Save", &config.AutoSaveEnabled);
				ImGui::DragFloat("Interval (s)", &config.AutoSaveInterval, 1.0f, 10.0f, 3600.0f, "%.0f");
			}
			else if (selectedCategory == 5) // Startup
			{
				ImGui::TextDisabled("Startup");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Load Last Project on Startup", &config.LoadLastProjectOnStartup);
				ImGui::Spacing();
				ImGui::TextDisabled("Last project:");
				ImGui::SameLine();
				ImGui::TextWrapped("%s", config.LastProjectPath.empty() ? "(none)" : config.LastProjectPath.c_str());
			}
			else if (selectedCategory == 6) // General
			{
				ImGui::TextDisabled("General");
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Confirm on Scene Close", &config.ConfirmOnSceneClose);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Show a warning when closing/switching a scene with unsaved changes.");
				}
				ImGui::DragInt("Max Recent Projects", &config.MaxRecentProjects, 1, 1, 50);
			}

			ImGui::PopStyleVar();
			ImGui::EndChild();

			if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Settings", ImVec2(-1, 0)))
			{
				EditorLayer::Get().SaveConfig();
				EditorGUI::ApplyTheme();
				EditorLayer::Get().RequestEditorFontReload();
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Save and apply all settings.");
			}
		}
		ImGui::End();
	}

} // namespace Chained

#ifndef CH_EDITOR_SETTINGS_H
#define CH_EDITOR_SETTINGS_H

#include <string>
#include <vector>

namespace Chained
{
	struct EditorConfig
	{
		std::string LastProjectPath;
		std::string LastScenePath;
		bool LoadLastProjectOnStartup = false;
		bool AutoSaveEnabled = true;
		float AutoSaveInterval = 300.0f;
		std::vector<std::string> RecentProjects;

		// --- Appearance: editor UI font (rebuilt live via EditorLayer::ReloadEditorFonts) ---
		// FontPath is relative to the engine resources root, prefixed with "engine/"
		// (e.g. "engine/resources/font/lato/lato-bold.ttf"). Empty means "use the built-in default".
		std::string FontPath = "engine/resources/font/lato/lato-bold.ttf";
		float FontSize = 16.0f;

		// --- Viewport gizmo icons (camera/light/spawn/audio billboards) ---
		// Screen-constant sizing: size = clamp(distanceToCamera * IconSizeScale, IconSizeMin, IconSizeMax).
		float IconSizeScale = 0.12f;
		float IconSizeMin = 1.2f;
		float IconSizeMax = 8.0f;

		// --- Editor camera (edit mode). Global, not per-project. ---
		float CameraMoveSpeed = 10.0f;
		float CameraBoostMultiplier = 5.0f;
		bool DisableCameraZoom = false;
		float CameraRotationSpeed = 1.0f;
		float CameraZoomSpeedMultiplier = 1.0f;
		float CameraFovDegrees = 45.0f;
		float CameraNearClip = 0.1f;
		float CameraFarClip = 10000.0f;

		// --- Viewport ---
		bool ShowEditorIcons = true;
		float GizmoScale = 1.0f;

		// --- Content Browser ---
		float DefaultThumbnailSize = 96.0f;
		int DefaultSortOrder = 0;
		bool ShowFileExtensions = true;

		// --- General ---
		bool ConfirmOnSceneClose = true;
		int MaxRecentProjects = 10;
	};
} // namespace Chained

#endif // CH_EDITOR_SETTINGS_H

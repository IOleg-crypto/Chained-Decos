#ifndef CH_EDITOR_COLORS_H
#define CH_EDITOR_COLORS_H

#include "imgui.h"

namespace Chained
{
	namespace EditorColors
	{
		// Toolbar & buttons
		inline constexpr ImVec4 TransparentButton = {0.1f, 0.1f, 0.1f, 0.0f};
		inline constexpr ImVec4 ActiveToolOrange = {0.9f, 0.45f, 0.0f, 1.0f};
		inline constexpr ImVec4 PlayGreen = {0.3f, 1.0f, 0.3f, 1.0f};
		inline constexpr ImVec4 SimulateOrange = {1.0f, 0.64f, 0.0f, 1.0f};
		inline constexpr ImVec4 ActiveSnapBlue = {0.3f, 0.8f, 1.0f, 1.0f};

		// Panel backgrounds
		inline constexpr ImVec4 ToolbarBg = {0.1f, 0.1f, 0.12f, 0.8f};
		inline constexpr ImVec4 FloatingToolbarBg = {0.1f, 0.1f, 0.12f, 0.0f};

		// Selection & highlights
		inline constexpr ImVec4 SelectionYellow = {1.0f, 1.0f, 0.0f, 1.0f};
		inline constexpr ImVec4 SelectionGreen = {0.0f, 1.0f, 0.0f, 1.0f};

		// Loading overlay
		inline constexpr ImVec4 LoadingOverlayBg = {0.02f, 0.02f, 0.02f, 0.92f};

		// Content browser / project selector
		inline constexpr ImVec4 SidebarBg = {0.15f, 0.15f, 0.18f, 1.0f};
		inline constexpr ImVec4 DarkPanelBg = {0.02f, 0.02f, 0.02f, 1.0f};
		inline constexpr ImVec4 ProjectCardBg = {0.12f, 0.12f, 0.13f, 1.0f};
		inline constexpr ImVec4 ProjectCardHover = {0.18f, 0.18f, 0.19f, 1.0f};
		inline constexpr ImVec4 ProjectCardActive = {0.1f, 0.1f, 0.1f, 1.0f};
		inline constexpr ImVec4 ProjectCardBorder = {0.18f, 0.18f, 0.20f, 1.0f};
		inline constexpr ImVec4 ProjectCardBorderHover = {0.26f, 0.26f, 0.28f, 1.0f};
		inline constexpr ImVec4 ProjectCardBorderActive = {0.14f, 0.14f, 0.16f, 1.0f};
		inline constexpr ImVec4 SubCardBg = {0.06f, 0.06f, 0.07f, 1.0f};
		inline constexpr ImVec4 SubCardBorder = {0.14f, 0.14f, 0.16f, 1.0f};
		inline constexpr ImVec4 SubCardBorderHover = {0.22f, 0.22f, 0.24f, 1.0f};
		inline constexpr ImVec4 SubCardBorderActive = {0.10f, 0.10f, 0.12f, 1.0f};
		inline constexpr ImVec4 PrimaryButton = {0.13f, 0.45f, 0.80f, 1.0f};
		inline constexpr ImVec4 PrimaryButtonHover = {0.20f, 0.55f, 0.92f, 1.0f};
		inline constexpr ImVec4 PrimaryButtonActive = {0.10f, 0.38f, 0.70f, 1.0f};
		inline constexpr ImVec4 MutedText = {0.5f, 0.5f, 0.5f, 1.0f};
		inline constexpr ImVec4 BrightText = {0.9f, 0.9f, 0.9f, 1.0f};
	} // namespace EditorColors
} // namespace Chained

#endif // CH_EDITOR_COLORS_H

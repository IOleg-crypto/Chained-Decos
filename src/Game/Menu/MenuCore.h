#ifndef MENU_CORE_H
#define MENU_CORE_H

#include "Menu.h"
#include <imgui.h>

// Core menu functionality - initialization, state management, settings
class MenuCore
{
public:
    // Core functionality
    static void Initialize(Menu* menu);
    static void Update(Menu* menu);
    static void Render(Menu* menu);
    
    // Action handling
    static void ExecuteAction(Menu* menu);
    static MenuAction GetAction(Menu* menu);
    static void SetAction(Menu* menu, MenuAction type);
    
    // Settings management
    static void LoadSettings(Menu* menu);
    static void SaveSettings(Menu* menu);
    
    // State management
    static void SetGameInProgress(Menu* menu, bool inProgress);

    // ImGui-specific functionality
    static void SetupImGuiStyle();
    static void SetupModernDarkTheme();
    static void BeginImGuiFrame();
    static void EndImGuiFrame();
    
    // Theme management
    static void ApplyCustomTheme(Menu* menu);
    static void ResetToDefaultTheme();
    
    // Utility functions
    static bool IsMenuVisible(Menu* menu);
    static void ToggleMenuVisibility(Menu* menu);
};

#endif // MENU_CORE_H
#ifndef MENU_CORE_H
#define MENU_CORE_H

#include "Menu.h"

// Core menu functionality - initialization, state management, settings
class MenuCore
{
public:
    static void Initialize(Menu* menu);
    static void Update(Menu* menu);
    static void ExecuteAction(Menu* menu);
    static void LoadSettings(Menu* menu);
    static void SaveSettings(Menu* menu);
    static void SetGameInProgress(Menu* menu, bool inProgress);
    static MenuAction GetAction(Menu* menu);
    static void SetAction(Menu* menu, MenuAction type);
};

#endif // MENU_CORE_H
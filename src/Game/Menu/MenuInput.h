#ifndef MENU_INPUT_H
#define MENU_INPUT_H

#include "Menu.h"

// Input handling functionality
class MenuInput
{
public:
    static void HandleKeyboardNavigation(Menu* menu);
    static void HandleMouseSelection(Menu* menu);
    static void HandleMainMenuKeyboardNavigation(Menu* menu);
    static void HandleVideoMenuKeyboardNavigation(Menu* menu);
    static void HandleGameplayNavigation(Menu* menu);
    static void HandleSimpleScreenKeyboardNavigation(Menu* menu);
    static void HandleMapSelectionKeyboardNavigation(Menu* menu);
    static void HandleConfirmExitKeyboardNavigation(Menu* menu);
    static void HandleConfirmExit(Menu* menu);
    static void HandleMapSelection(Menu* menu);
};

#endif // MENU_INPUT_H
#ifndef MAIN_MENU_SCREEN_H
#define MAIN_MENU_SCREEN_H

#include "BaseMenuScreen.h"

class MainMenuScreen : public BaseMenuScreen
{
public:
    void Update() override;
    void Render() override;
    const char *GetTitle() const override
    {
        return "CHAINED DECOS";
    }
};

#endif // MAIN_MENU_SCREEN_H

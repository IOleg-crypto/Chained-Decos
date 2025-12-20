#ifndef CONFIRM_EXIT_SCREEN_H
#define CONFIRM_EXIT_SCREEN_H

#include "BaseMenuScreen.h"

class ConfirmExitScreen : public BaseMenuScreen
{
public:
    void Update() override
    {
    }
    void Render() override;
    const char *GetTitle() const override
    {
        return "EXIT GAME?";
    }
};

#endif // CONFIRM_EXIT_SCREEN_H

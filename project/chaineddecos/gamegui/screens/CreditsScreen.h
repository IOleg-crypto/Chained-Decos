#ifndef CREDITS_SCREEN_H
#define CREDITS_SCREEN_H

#include "BaseMenuScreen.h"

class CreditsScreen : public BaseMenuScreen
{
public:
    void Update() override
    {
    }
    void Render() override;
    const char *GetTitle() const override
    {
        return "CREDITS";
    }
};

#endif // CREDITS_SCREEN_H

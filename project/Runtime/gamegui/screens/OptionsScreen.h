#ifndef OPTIONS_SCREEN_H
#define OPTIONS_SCREEN_H

#include "BaseMenuScreen.h"

class OptionsScreen : public BaseMenuScreen
{
public:
    void Update() override
    {
    }
    void Render() override;
    const char *GetTitle() const override
    {
        return "OPTIONS";
    }
};

#endif // OPTIONS_SCREEN_H

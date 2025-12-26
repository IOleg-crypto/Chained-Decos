#ifndef SETTINGS_SCREENS_H
#define SETTINGS_SCREENS_H

#include "BaseMenuScreen.h"

class VideoSettingsScreen : public BaseMenuScreen
{
public:
    void Update() override
    {
    }
    void Render() override;
    const char *GetTitle() const override
    {
        return "VIDEO SETTINGS";
    }
};

class AudioSettingsScreen : public BaseMenuScreen
{
public:
    void Update() override
    {
    }
    void Render() override;
    const char *GetTitle() const override
    {
        return "AUDIO SETTINGS";
    }
};

class ControlSettingsScreen : public BaseMenuScreen
{
public:
    void Update() override
    {
    }
    void Render() override;
    const char *GetTitle() const override
    {
        return "CONTROL SETTINGS";
    }
};

#endif // SETTINGS_SCREENS_H

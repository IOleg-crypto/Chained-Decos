#ifndef IMENU_SCREEN_H
#define IMENU_SCREEN_H

#include "core/events/MenuEvent.h"
#include <string>

#include "core/interfaces/IMenu.h"

class IMenuScreen
{
public:
    virtual ~IMenuScreen() = default;

    virtual void Initialize(IMenu *menu) = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void HandleInput() = 0;
    virtual const char *GetTitle() const = 0;
};

#endif // IMENU_SCREEN_H



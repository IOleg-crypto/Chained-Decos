#ifndef MAP_SELECTION_SCREEN_H
#define MAP_SELECTION_SCREEN_H

#include "BaseMenuScreen.h"

class MapSelectionScreen : public BaseMenuScreen
{
public:
    void Update() override;
    void Render() override;
    const char *GetTitle() const override
    {
        return "MAP SELECTION";
    }
};

#endif // MAP_SELECTION_SCREEN_H

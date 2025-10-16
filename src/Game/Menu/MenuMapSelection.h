#ifndef MENU_MAP_SELECTION_H
#define MENU_MAP_SELECTION_H

#include "Menu.h"

// Map selection methods (moved from Menu.cpp for better organization)
void InitializeMaps();
void RenderMapSelection() const;
const MapInfo* GetSelectedMap() const;
std::string GetSelectedMapName() const;
void ScanForJsonMaps();

#endif // MENU_MAP_SELECTION_H
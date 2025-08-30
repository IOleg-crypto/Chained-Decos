//
// Created by I#Oleg.
//
#ifndef COLORPARSER_H
#define COLORPARSER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

// # ============================
// # Needed for parsing .json file
// # ============================
inline Color ParseColorByName(const std::string &name){
    // Hash table that contains all colors
    const std::unordered_map<std::string, Color> colorMap = {
        {"white", WHITE},
        {"black", BLACK},
        {"red", RED},
        {"green", GREEN},
        {"blue", BLUE},
        {"yellow", YELLOW},
        {"orange", ORANGE},
        {"gray", GRAY},
        {"lightgray", LIGHTGRAY},
        {"darkgray", DARKGRAY},
        {"purple", PURPLE},
        {"skyblue", SKYBLUE},
        {"magenta", MAGENTA},
        {"pink", PINK},
        {"beige", BEIGE},
        {"brown", BROWN},
        {"lime", LIME},
        {"maroon", MAROON},
        {"gold", GOLD}
    };

    if (const auto it = colorMap.find(name); it != colorMap.end()) {
        return it->second;
    }

    TraceLog(LOG_WARNING, "Unknown color name: %s", name.c_str());
    return WHITE; // default
}
#endif



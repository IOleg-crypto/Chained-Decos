#include "ColorParser.h"
#include <algorithm>
#include <cctype>

static const std::unordered_map<std::string, Color> colorMap = {
    {"white", WHITE},         {"black", BLACK},     {"red", RED},       {"green", GREEN},
    {"blue", BLUE},           {"yellow", YELLOW},   {"orange", ORANGE}, {"pink", PINK},
    {"purple", PURPLE},       {"brown", BROWN},     {"gray", GRAY},     {"darkgray", DARKGRAY},
    {"lightgray", LIGHTGRAY}, {"skyblue", SKYBLUE}, {"lime", LIME},     {"gold", GOLD},
    {"maroon", MAROON},       {"magenta", MAGENTA}, {"beige", BEIGE},   {"violet", VIOLET}};

Color ParseColorByName(const std::string &colorName)
{
    std::string lowerName = colorName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto it = colorMap.find(lowerName);
    if (it != colorMap.end())
    {
        return it->second;
    }

    return WHITE; // Default color
}

const std::unordered_map<std::string, Color> &GetColorMap()
{
    return colorMap;
}





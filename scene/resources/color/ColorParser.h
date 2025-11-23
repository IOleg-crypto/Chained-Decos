#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>

// Parse color by name (e.g., "red", "blue", "green")
Color ParseColorByName(const std::string &colorName);

// Get all available color names
const std::unordered_map<std::string, Color> &GetColorMap();

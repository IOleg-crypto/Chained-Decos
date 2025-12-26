#ifndef MENU_CONSTANTS_H
#define MENU_CONSTANTS_H

#include <cstdint>
#include <string>
#include <vector>

namespace MenuConstants
{

// Layout constants
constexpr int MAPS_PER_PAGE = 12; // 3 rows × 4 columns
constexpr int MAP_BOX_WIDTH = 280;
constexpr int MAP_BOX_HEIGHT = 160;
constexpr int MARGIN = 20;
constexpr int TOP_MARGIN = 150;

// Font sizes
constexpr int TITLE_FONT_SIZE = 64;
constexpr int NAME_FONT_SIZE = 32;
constexpr int DESCRIPTION_FONT_SIZE = 20;
constexpr int INSTRUCTIONS_FONT_SIZE = 18;
constexpr int PAGE_INFO_FONT_SIZE = 24;

// Console constants
constexpr size_t MAX_CONSOLE_LINES = 100;
constexpr size_t MAX_HISTORY_LINES = 50;

// Default settings
constexpr float DEFAULT_MASTER_VOLUME = 1.0f;
constexpr float DEFAULT_MUSIC_VOLUME = 0.7f;
constexpr float DEFAULT_SFX_VOLUME = 0.8f;
constexpr float DEFAULT_MOUSE_SENSITIVITY = 1.0f;

// Parkour settings
constexpr float DEFAULT_WALL_RUN_SENSITIVITY = 1.0f;
constexpr float DEFAULT_JUMP_TIMING = 1.0f;
constexpr float DEFAULT_SLIDE_CONTROL = 1.0f;
constexpr float DEFAULT_GRAPPLE_SENSITIVITY = 1.0f;

// Gameplay settings
constexpr int DEFAULT_DIFFICULTY_LEVEL = 2; // Medium

// Map directories to search
const std::vector<std::string> MAP_SEARCH_DIRECTORIES = {"", "/maps", "/resources/maps"};

// Default colors
const std::string DEFAULT_FONT_PATH = "/resources/font/AlanSans.ttf";
const std::string FALLBACK_MAP_NAME = "parkour_test";
const std::string FALLBACK_MAP_DISPLAY_NAME = "Built-in Parkour";
const std::string FALLBACK_MAP_DESCRIPTION = "Default parkour level with basic platforming";

// Video options
const std::vector<std::string> RESOLUTION_OPTIONS = {"1920x1080", "1280x720", "1366x768",
                                                     "1600x900", "2560x1440"};

const std::vector<std::string> ASPECT_RATIO_OPTIONS = {"16:9", "4:3", "21:9"};

const std::vector<std::string> DISPLAY_MODE_OPTIONS = {"Windowed", "Fullscreen", "Borderless"};

const std::vector<std::string> VSYNC_OPTIONS = {"Off", "On"};

const std::vector<std::string> FPS_OPTIONS = {"30",  "60",  "120", "144",
                                              "165", "180", "240", "Unlimited"};

// Gameplay options
const std::vector<std::string> DIFFICULTY_OPTIONS = {"Easy", "Medium", "Hard"};

const std::vector<std::string> BOOLEAN_OPTIONS = {"Off", "On"};

} // namespace MenuConstants

#endif // MENU_CONSTANTS_H




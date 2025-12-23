#ifndef MAPDATA_H
#define MAPDATA_H

#include <cstdint>
#include <raylib.h>
#include <string>

// ============================================================================
// Enumerations
// ============================================================================

enum class MapObjectType : uint8_t
{
    CUBE = 0,
    SPHERE = 1,
    CYLINDER = 2,
    PLANE = 3,
    LIGHT = 4,
    MODEL = 5,
    SPAWN_ZONE = 6
};

enum class SceneType : uint8_t
{
    LEVEL_3D = 0, // Standard map with geometry
    UI_MENU = 1,  // UI-focused scene (e.g. Main Menu)
    EMPTY = 2     // Blank slate
};

// ============================================================================
// Data Structures
// ============================================================================

struct MapObjectData
{
    std::string name;
    MapObjectType type;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Color color;
    std::string modelName; // For MODEL type objects

    // Shape-specific properties
    float radius; // For spheres
    float height; // For cylinders
    Vector2 size; // For planes

    // Collision properties
    bool isPlatform;
    bool isObstacle;
};

// UI Element Data for serialization
struct UIElementData
{
    std::string name;
    std::string type; // "button", "text", "image"
    bool isActive = true;

    // RectTransform data
    int anchor = 0; // UIAnchor enum as int
    Vector2 position = {0, 0};
    Vector2 size = {100, 100};
    Vector2 pivot = {0.5f, 0.5f};
    float rotation = 0.0f;

    // Component-specific properties (stored as key-value pairs)
    std::string text;        // For UIText
    std::string fontName;    // For UIText
    int fontSize = 20;       // For UIText
    float spacing = 1.0f;    // For UIText
    Color textColor = WHITE; // For UIText

    Color normalColor = GRAY;      // For UIButton
    Color hoverColor = LIGHTGRAY;  // For UIButton
    Color pressedColor = DARKGRAY; // For UIButton
    float borderRadius = 0.0f;     // For UIButton and UIImage
    float borderWidth = 0.0f;      // For UIButton and UIImage
    Color borderColor = BLACK;     // For UIButton and UIImage
    std::string eventId;           // For UIButton

    Color tint = WHITE;      // For UIImage
    std::string texturePath; // For UIImage (future)

    // Action System
    std::string actionType;   // "None", "LoadScene", "Quit", "OpenURL"
    std::string actionTarget; // Scene path or URL
};

struct MapMetadata
{
    std::string name;
    std::string displayName;
    std::string description;
    std::string author;
    std::string version;
    Vector3 startPosition;
    Vector3 endPosition;
    Color skyColor;
    Color groundColor;
    float difficulty;

    // Additional fields for JSON export compatibility
    std::string createdDate;
    std::string modifiedDate;
    Vector3 worldBounds;
    Color backgroundColor;
    std::string skyboxTexture;
    SceneType sceneType;
};

#endif /* MAPDATA_H */

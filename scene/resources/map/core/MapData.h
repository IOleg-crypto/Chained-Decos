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

    // RectTransform data
    int anchor; // UIAnchor enum as int
    Vector2 position;
    Vector2 size;
    Vector2 pivot;
    float rotation;

    // Component-specific properties (stored as key-value pairs)
    std::string text;     // For UIText
    std::string fontName; // For UIText
    int fontSize;         // For UIText
    float spacing;        // For UIText
    Color textColor;      // For UIText

    Color normalColor;   // For UIButton
    Color hoverColor;    // For UIButton
    Color pressedColor;  // For UIButton
    float borderRadius;  // For UIButton and UIImage
    float borderWidth;   // For UIButton and UIImage
    Color borderColor;   // For UIButton and UIImage
    std::string eventId; // For UIButton

    Color tint;              // For UIImage
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

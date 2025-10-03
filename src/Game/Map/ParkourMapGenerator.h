#ifndef PARKOUR_MAP_GENERATOR_H
#define PARKOUR_MAP_GENERATOR_H

#include <raylib.h>
#include <string>
#include <vector>

// Parkour shape types using raylib's built-in geometry
enum class ParkourShapeType
{
    Cube,
    Sphere,
    Cylinder,
    Plane,
    Capsule,
    Torus
};

// Individual parkour element
struct ParkourElement
{
    ParkourShapeType type;
    Vector3 position;
    Vector3 size;
    Color color;
    bool isPlatform;
    bool isObstacle;
    bool isMoving; // For future moving platforms
    float moveSpeed; // For future moving platforms
    Vector3 moveDirection; // For future moving platforms
};

// Parkour test map definition
struct ParkourTestMap
{
    std::string name;
    std::string displayName;
    std::string description;
    std::vector<ParkourElement> elements;
    Vector3 startPosition;
    Vector3 endPosition;
    Color skyColor;
    Color groundColor;
    float difficulty; // 1.0 = Easy, 5.0 = Expert
};

// Parkour map generator class
class ParkourMapGenerator
{
private:
    static ParkourTestMap CreateBasicShapesMap();
    static ParkourTestMap CreateGeometricChallengeMap();
    static ParkourTestMap CreatePrecisionPlatformingMap();
    static ParkourTestMap CreateVerticalAscensionMap();
    static ParkourTestMap CreateSpeedRunnersGauntletMap();
    static ParkourTestMap CreateShapeTrainingGroundMap();

public:
    // Get all available parkour test maps
    static std::vector<ParkourTestMap> GetAllParkourMaps();

    // Get a specific map by name
    static ParkourTestMap GetMapByName(const std::string& name);

    // Render a parkour map (for preview or gameplay)
    static void RenderParkourMap(const ParkourTestMap& map, Camera3D camera);

    // Helper function to create a cube element
    static ParkourElement CreateCube(const Vector3& position, const Vector3& size, const Color& color, bool isPlatform = true);

    // Helper function to create a sphere element
    static ParkourElement CreateSphere(const Vector3& position, float radius, const Color& color, bool isPlatform = true);

    // Helper function to create a cylinder element
    static ParkourElement CreateCylinder(const Vector3& position, float radius, float height, const Color& color, bool isPlatform = true);

    // Helper function to create a platform element
    static ParkourElement CreatePlatform(const Vector3& position, const Vector3& size, const Color& color);
};

#endif // PARKOUR_MAP_GENERATOR_H
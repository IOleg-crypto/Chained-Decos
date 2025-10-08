#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <raylib.h>
#include <string>

// Structure for map objects (cubes, spheres, cylinders , etc)
class MapObject
{
private:
    Vector3 position;           // Object position in 3D space
    Vector3 scale;              // Object scale (size)
    Vector3 rotation;           // Object rotation in radians
    Color color;                // Object color
    std::string objectName;     // Object name for identification
    int objectType;             // Object type: 0=cube, 1=sphere, 2=cylinder, 3=plane, 4=ellipse, 5=model
    bool isSelected;            // Selection state for UI
    Vector2 planeSize;          // Size for plane objects
    // For ellipse
    float verticalRadius;
    float horizontalRadius;
    float sphereRadius;
    // For models
    std::string modelAssetName; // Name of the 3D model asset (for type=5)

public:
    MapObject();
    ~MapObject();

    // Getters
    [[nodiscard]] Vector3 GetPosition() const;
    [[nodiscard]] Vector3 GetScale() const;
    [[nodiscard]] Vector3 GetRotation() const;
    [[nodiscard]] Color GetColor() const;
    [[nodiscard]] std::string GetObjectName() const;
    [[nodiscard]] int GetObjectType() const;
    [[nodiscard]] bool IsSelected() const;
    [[nodiscard]] Vector2 GetPlaneSize() const;
    [[nodiscard]] float GetVerticalRadius() const;
    [[nodiscard]] float GetHorizontalRadius() const;
    [[nodiscard]] float GetSphereRadius() const;
    [[nodiscard]] std::string GetModelAssetName() const;

    // Setters
    void SetPosition(const Vector3 &pos);
    void SetScale(const Vector3 &scale);
    void SetRotation(const Vector3 &rot);
    void SetColor(const Color &col);
    void SetObjectName(const std::string &objectName);
    void SetObjectType(int objectType);
    void SetSelected(bool isSelected);
    void SetPlaneSize(const Vector2 &planeSize);
    void SetVerticalRadius(float verticalRadius);
    void SetHorizontalRadius(float horizontalRadius);
    void SetSphereRadius(float sphereRadius);
    void SetModelAssetName(const std::string &modelAssetName);
};

#endif
#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <raylib.h>
#include <string>

// Structure for map objects (cubes, spheres, cylinders , etc)
class MapObject
{
private:
    Vector3 position; // Object position in 3D space
    Vector3 scale;    // Object scale (size)
    Vector3 rotation; // Object rotation in radians
    Color color;      // Object color
    std::string name; // Object name for identification
    int type;         // Object type: 0=cube, 1=sphere, 2=cylinder, 3=plane, 4=ellipse, 5=model
    bool selected;    // Selection state for UI
    Vector2 size;     // If it`s plane or other meshes
    // For ellipse
    float radiusV;
    float radiusH;
    float radiusSphere;
    // For models
    std::string modelName; // Name of the model (for type=5)

public:
    MapObject();
    ~MapObject();

    // Getters
    [[nodiscard]] Vector3 GetPosition() const;
    [[nodiscard]] Vector3 GetScale() const;
    [[nodiscard]] Vector3 GetRotation() const;
    [[nodiscard]] Color GetColor() const;
    [[nodiscard]] std::string GetName() const;
    [[nodiscard]] int &GetType();
    [[nodiscard]] int GetType() const;
    [[nodiscard]] bool IsSelected() const;
    [[nodiscard]] Vector2 GetSize() const;
    [[nodiscard]] float GetRadiusV() const;
    [[nodiscard]] float GetRadiusH() const;
    [[nodiscard]] float GetRadiusSphere() const;
    [[nodiscard]] bool GetSelected() const;
    [[nodiscard]] std::string GetModelName() const;

    // Setters
    void SetPosition(const Vector3 &pos);
    void SetScale(const Vector3 &scale);
    void SetRotation(const Vector3 &rot);
    void SetColor(const Color &col);
    void SetName(const std::string &name);
    void SetType(int type);
    void SetSelected(bool selected);
    void SetSize(const Vector2 &size);
    void SetRadiusV(float radiusV);
    void SetRadiusH(float radiusH);
    void SetRadiusSphere(float radiusSphere);
    void SetModelName(const std::string &modelName);
};

#endif
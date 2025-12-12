#pragma once

#include "Component.h"
#include <raylib.h>
#include <string>

enum class MeshType : uint8_t
{
    Cube = 0,
    Sphere = 1,
    Cylinder = 2,
    Plane = 3,
    Ellipse = 4,
    Model = 5,
    SpawnZone = 6
};

class MeshRendererComponent : public Component
{
public:
    MeshRendererComponent(GameObject *owner);

    void OnRender() override;
    void OnInspectorGUI() override;
    void Serialize(nlohmann::json &json) const override;
    void Deserialize(const nlohmann::json &json) override;

    // Accessors
    void SetModelAssetName(const std::string &name);
    std::string GetModelAssetName() const;

private:
    MeshType m_type;
    Color m_color;
    float m_sphereRadius;
    Vector2 m_planeSize;
    Vector2 m_ellipseRadius;
    std::string m_modelAssetName;
};

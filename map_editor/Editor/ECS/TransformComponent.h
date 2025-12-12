#pragma once

#include "Component.h"
#include <raylib.h>
#include <raymath.h>

class TransformComponent : public Component
{
public:
    TransformComponent(GameObject *owner);

    void OnInspectorGUI() override;
    void Serialize(nlohmann::json &json) const override;
    void Deserialize(const nlohmann::json &json) override;

    // Getters/Setters
    Vector3 GetPosition() const;
    void SetPosition(Vector3 pos);

    Vector3 GetRotation() const;
    void SetRotation(Vector3 rot);

    Vector3 GetScale() const;
    void SetScale(Vector3 scale);

    // Helper to get Transform matrix
    Matrix GetGlobalTransform() const;

private:
    Vector3 m_position;
    Vector3 m_rotation; // Euler angles in radians
    Vector3 m_scale;
};

#include "TransformComponent.h"
#include <imgui.h>

TransformComponent::TransformComponent(GameObject *owner)
    : Component(owner), m_position{0.0f, 0.0f, 0.0f}, m_rotation{0.0f, 0.0f, 0.0f},
      m_scale{1.0f, 1.0f, 1.0f}
{
}

void TransformComponent::OnInspectorGUI()
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float pos[3] = {m_position.x, m_position.y, m_position.z};
        if (ImGui::DragFloat3("Position", pos, 0.1f))
        {
            m_position = {pos[0], pos[1], pos[2]};
        }

        float rot[3] = {m_rotation.x * RAD2DEG, m_rotation.y * RAD2DEG, m_rotation.z * RAD2DEG};
        if (ImGui::DragFloat3("Rotation", rot, 1.0f))
        {
            m_rotation = {rot[0] * DEG2RAD, rot[1] * DEG2RAD, rot[2] * DEG2RAD};
        }

        float scale[3] = {m_scale.x, m_scale.y, m_scale.z};
        if (ImGui::DragFloat3("Scale", scale, 0.1f))
        {
            m_scale = {scale[0], scale[1], scale[2]};
        }
    }
}

void TransformComponent::Serialize(nlohmann::json &json) const
{
    json["position"] = {m_position.x, m_position.y, m_position.z};
    json["rotation"] = {m_rotation.x, m_rotation.y, m_rotation.z};
    json["scale"] = {m_scale.x, m_scale.y, m_scale.z};
}

void TransformComponent::Deserialize(const nlohmann::json &json)
{
    if (json.contains("position") && json["position"].is_array() && json["position"].size() == 3)
    {
        m_position.x = json["position"][0].get<float>();
        m_position.y = json["position"][1].get<float>();
        m_position.z = json["position"][2].get<float>();
    }
    if (json.contains("rotation") && json["rotation"].is_array() && json["rotation"].size() == 3)
    {
        m_rotation.x = json["rotation"][0].get<float>();
        m_rotation.y = json["rotation"][1].get<float>();
        m_rotation.z = json["rotation"][2].get<float>();
    }
    if (json.contains("scale") && json["scale"].is_array() && json["scale"].size() == 3)
    {
        m_scale.x = json["scale"][0].get<float>();
        m_scale.y = json["scale"][1].get<float>();
        m_scale.z = json["scale"][2].get<float>();
    }
}

Vector3 TransformComponent::GetPosition() const
{
    return m_position;
}

void TransformComponent::SetPosition(Vector3 pos)
{
    m_position = pos;
}

Vector3 TransformComponent::GetRotation() const
{
    return m_rotation;
}

void TransformComponent::SetRotation(Vector3 rot)
{
    m_rotation = rot;
}

Vector3 TransformComponent::GetScale() const
{
    return m_scale;
}

void TransformComponent::SetScale(Vector3 scale)
{
    m_scale = scale;
}

Matrix TransformComponent::GetGlobalTransform() const
{
    Matrix matScale = MatrixScale(m_scale.x, m_scale.y, m_scale.z);
    Matrix matRotation = MatrixRotateXYZ(m_rotation);
    Matrix matTranslation = MatrixTranslate(m_position.x, m_position.y, m_position.z);

    return MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);
}

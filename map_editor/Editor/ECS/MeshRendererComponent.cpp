#include "MeshRendererComponent.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include <imgui.h>
#include <rlgl.h>


MeshRendererComponent::MeshRendererComponent(GameObject *owner)
    : Component(owner), m_type(MeshType::Cube), m_color(WHITE), m_sphereRadius(1.0f),
      m_planeSize{10.0f, 10.0f}, m_ellipseRadius{1.0f, 1.0f}
{
}

void MeshRendererComponent::OnRender()
{
    auto transform = m_owner->GetComponent<TransformComponent>();
    if (!transform)
        return;

    Vector3 pos = transform->GetPosition();
    Vector3 rot = transform->GetRotation(); // In radians
    Vector3 scale = transform->GetScale();

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(rot.z * RAD2DEG, 0, 0, 1); // Roll
    rlRotatef(rot.y * RAD2DEG, 0, 1, 0); // Yaw
    rlRotatef(rot.x * RAD2DEG, 1, 0, 0); // Pitch
    rlScalef(scale.x, scale.y, scale.z);

    switch (m_type)
    {
    case MeshType::Cube:
        DrawCube(Vector3Zero(), 1.0f, 1.0f, 1.0f, m_color);
        DrawCubeWires(Vector3Zero(), 1.0f, 1.0f, 1.0f, DARKGRAY);
        break;
    case MeshType::Sphere:
        DrawSphere(Vector3Zero(), m_sphereRadius, m_color);
        DrawSphereWires(Vector3Zero(), m_sphereRadius, 16, 16, DARKGRAY);
        break;
    case MeshType::Cylinder:
        DrawCylinder(Vector3Zero(), 1.0f, 1.0f, 1.0f, 16, m_color);
        DrawCylinderWires(Vector3Zero(), 1.0f, 1.0f, 1.0f, 16, DARKGRAY);
        break;
    case MeshType::Plane:
        DrawPlane(Vector3Zero(), m_planeSize, m_color);
        break;
    case MeshType::Model:
        DrawCube(Vector3Zero(), 1.0f, 1.0f, 1.0f, PURPLE);
        DrawCubeWires(Vector3Zero(), 1.0f, 1.0f, 1.0f, YELLOW);
        break;
    default:
        break;
    }

    rlPopMatrix();
}

void MeshRendererComponent::OnInspectorGUI()
{
    if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Type Selection
        const char *types[] = {"Cube",    "Sphere", "Cylinder",  "Plane",
                               "Ellipse", "Model",  "Spawn Zone"};
        int currentType = static_cast<int>(m_type);
        if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
        {
            m_type = static_cast<MeshType>(currentType);
        }

        // Color
        float color[4] = {m_color.r / 255.0f, m_color.g / 255.0f, m_color.b / 255.0f,
                          m_color.a / 255.0f};
        if (ImGui::ColorEdit4("Color", color))
        {
            m_color = {(unsigned char)(color[0] * 255), (unsigned char)(color[1] * 255),
                       (unsigned char)(color[2] * 255), (unsigned char)(color[3] * 255)};
        }

        // Type specific
        if (m_type == MeshType::Sphere)
        {
            ImGui::DragFloat("Radius", &m_sphereRadius, 0.1f);
        }
        else if (m_type == MeshType::Plane)
        {
            float size[2] = {m_planeSize.x, m_planeSize.y};
            if (ImGui::DragFloat2("Size", size, 0.1f))
            {
                m_planeSize = {size[0], size[1]};
            }
        }
        else if (m_type == MeshType::Model)
        {
            char buffer[128];
            strncpy(buffer, m_modelAssetName.c_str(), sizeof(buffer));
            if (ImGui::InputText("Model Asset", buffer, sizeof(buffer)))
            {
                m_modelAssetName = std::string(buffer);
            }
        }
    }
}

void MeshRendererComponent::Serialize(nlohmann::json &json) const
{
    json["type"] = static_cast<int>(m_type);
    json["color"] = {m_color.r, m_color.g, m_color.b, m_color.a};
    json["sphereRadius"] = m_sphereRadius;
    json["planeSizeX"] = m_planeSize.x;
    json["planeSizeY"] = m_planeSize.y;
    json["modelAsset"] = m_modelAssetName;
}

void MeshRendererComponent::Deserialize(const nlohmann::json &json)
{
    if (json.contains("type"))
        m_type = static_cast<MeshType>(json["type"].get<int>());

    if (json.contains("color") && json["color"].is_array() && json["color"].size() >= 4)
    {
        m_color.r = json["color"][0].get<unsigned char>();
        m_color.g = json["color"][1].get<unsigned char>();
        m_color.b = json["color"][2].get<unsigned char>();
        m_color.a = json["color"][3].get<unsigned char>();
    }

    if (json.contains("sphereRadius"))
        m_sphereRadius = json["sphereRadius"].get<float>();
    if (json.contains("planeSizeX"))
        m_planeSize.x = json["planeSizeX"].get<float>();
    if (json.contains("planeSizeY"))
        m_planeSize.y = json["planeSizeY"].get<float>();
    if (json.contains("modelAsset"))
        m_modelAssetName = json["modelAsset"].get<std::string>();
}

void MeshRendererComponent::SetModelAssetName(const std::string &name)
{
    m_modelAssetName = name;
}
std::string MeshRendererComponent::GetModelAssetName() const
{
    return m_modelAssetName;
}

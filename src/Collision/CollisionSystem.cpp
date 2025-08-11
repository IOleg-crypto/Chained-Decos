//
// Created by I#Oleg
//
#include <Collision/CollisionSystem.h>
#include <raylib.h>

Collision::Collision(const Vector3 &center, const Vector3 &size) { Update(center, size); }

void Collision::Update(const Vector3 &center, const Vector3 &size)
{
    m_min = Vector3Subtract(center, Vector3Scale(size, 0.5f));
    m_max = Vector3Add(center, Vector3Scale(size, 0.5f));
}

bool Collision::Intersects(const Collision &other) const
{
    return (m_min.x <= other.m_max.x && m_max.x >= other.m_min.x) &&
           (m_min.y <= other.m_max.y && m_max.y >= other.m_min.y) &&
           (m_min.z <= other.m_max.z && m_max.z >= other.m_min.z);
}

bool Collision::Contains(const Vector3 &point) const
{
    return (point.x >= m_min.x && point.x <= m_max.x) &&
           (point.y >= m_min.y && point.y <= m_max.y) && (point.z >= m_min.z && point.z <= m_max.z);
}
Vector3 Collision::GetMin() const { return m_min; }
Vector3 Collision::GetMax() const { return m_max; }

void Collision::GetMeshModelCollision(Model *model)
{
    if (!model || model->meshCount == 0)
        return;

    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    // Проходимо по всіх мешах моделі
    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];

        // Проходимо по всіх вершинах у меші
        for (int i = 0; i < mesh.vertexCount; i++)
        {
            Vector3 v = {mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 1],
                         mesh.vertices[i * 3 + 2]};

            if (v.x < min.x)
                min.x = v.x;
            if (v.y < min.y)
                min.y = v.y;
            if (v.z < min.z)
                min.z = v.z;

            if (v.x > max.x)
                max.x = v.x;
            if (v.y > max.y)
                max.y = v.y;
            if (v.z > max.z)
                max.z = v.z;
        }
    }

    // Обчислюємо центр і розміри bounding box
    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

    // Оновлюємо внутрішні межі колізії
    Update(center, size);

    // Малюємо bounding box
    DrawCubeWires(center, size.x, size.y, size.z, GREEN);
}

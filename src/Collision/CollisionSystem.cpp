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

// ✅ SRP: тільки обчислення, без рендерингу
void Collision::CalculateFromModel(Model *model)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for collision calculation");
        return;
    }

    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];
        for (int i = 0; i < mesh.vertexCount; i++)
        {
            Vector3 v = {mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 1],
                         mesh.vertices[i * 3 + 2]};

            min.x = fminf(min.x, v.x);
            min.y = fminf(min.y, v.y);
            min.z = fminf(min.z, v.z);

            max.x = fmaxf(max.x, v.x);
            max.y = fmaxf(max.y, v.y);
            max.z = fmaxf(max.z, v.z);
        }
    }

    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

    Update(center, size);

    TraceLog(LOG_INFO, "Model collision calculated: center=(%.2f,%.2f,%.2f) size=(%.2f,%.2f,%.2f)",
             center.x, center.y, center.z, size.x, size.y, size.z);
}

// ✅ SRP: обчислення з трансформацією
void Collision::CalculateFromModel(Model *model, const Matrix &transform)
{
    if (!model || model->meshCount == 0)
    {
        TraceLog(LOG_WARNING, "Invalid model provided for transformed collision calculation");
        return;
    }

    Vector3 min = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int m = 0; m < model->meshCount; m++)
    {
        Mesh &mesh = model->meshes[m];
        for (int i = 0; i < mesh.vertexCount; i++)
        {
            Vector3 v = {mesh.vertices[i * 3 + 0], mesh.vertices[i * 3 + 1],
                         mesh.vertices[i * 3 + 2]};

            // Apply transform matrix to vertex
            Vector3 transformedV = Vector3Transform(v, transform);

            min.x = fminf(min.x, transformedV.x);
            min.y = fminf(min.y, transformedV.y);
            min.z = fminf(min.z, transformedV.z);

            max.x = fmaxf(max.x, transformedV.x);
            max.y = fmaxf(max.y, transformedV.y);
            max.z = fmaxf(max.z, transformedV.z);
        }
    }

    Vector3 size = Vector3Subtract(max, min);
    Vector3 center = Vector3Add(min, Vector3Scale(size, 0.5f));

    Update(center, size);

    TraceLog(LOG_INFO,
             "Transformed collision calculated: center=(%.2f,%.2f,%.2f) size=(%.2f,%.2f,%.2f)",
             center.x, center.y, center.z, size.x, size.y, size.z);
}

// ✅ Реалізація нових getter методів для ООП
Vector3 Collision::GetCenter() const
{
    return Vector3Add(m_min, Vector3Scale(Vector3Subtract(m_max, m_min), 0.5f));
}

Vector3 Collision::GetSize() const { return Vector3Subtract(m_max, m_min); }

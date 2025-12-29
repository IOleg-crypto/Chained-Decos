#include "CollisionSpatialGrid.h"
#include <cmath>
#include <unordered_set>

CollisionSpatialGrid::CollisionSpatialGrid(float cellSize) : m_cellSize(cellSize)
{
}

void CollisionSpatialGrid::Clear()
{
    m_staticGrid.clear();
    m_entityGrid.clear();
}

void CollisionSpatialGrid::Build(const std::vector<std::shared_ptr<Collision>> &objects)
{
    m_staticGrid.clear();
    m_staticGrid.reserve(objects.size() * 4);

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto &obj = objects[i];
        Vector3 min = obj->GetMin();
        Vector3 max = obj->GetMax();

        int minX = static_cast<int>(floorf(min.x / m_cellSize));
        int maxX = static_cast<int>(floorf(max.x / m_cellSize));
        int minZ = static_cast<int>(floorf(min.z / m_cellSize));
        int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int z = minZ; z <= maxZ; ++z)
            {
                m_staticGrid[{x, z}].push_back(i);
            }
        }
    }
}

void CollisionSpatialGrid::BuildEntities(
    const std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> &entityColliders)
{
    m_entityGrid.clear();
    for (const auto &[entity, collider] : entityColliders)
    {
        Vector3 min = collider->GetMin();
        Vector3 max = collider->GetMax();

        int minX = static_cast<int>(floorf(min.x / m_cellSize));
        int maxX = static_cast<int>(floorf(max.x / m_cellSize));
        int minZ = static_cast<int>(floorf(min.z / m_cellSize));
        int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int z = minZ; z <= maxZ; ++z)
            {
                m_entityGrid[{x, z}].push_back(entity);
            }
        }
    }
}

std::vector<size_t> CollisionSpatialGrid::GetNearbyObjectIndices(const Collision &target) const
{
    Vector3 min = target.GetMin();
    Vector3 max = target.GetMax();

    int minX = static_cast<int>(floorf(min.x / m_cellSize));
    int maxX = static_cast<int>(floorf(max.x / m_cellSize));
    int minZ = static_cast<int>(floorf(min.z / m_cellSize));
    int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

    std::unordered_set<size_t> indices;
    for (int x = minX; x <= maxX; ++x)
    {
        for (int z = minZ; z <= maxZ; ++z)
        {
            GridKey key = {x, z};
            auto it = m_staticGrid.find(key);
            if (it != m_staticGrid.end())
            {
                for (size_t idx : it->second)
                {
                    indices.insert(idx);
                }
            }
        }
    }

    return std::vector<size_t>(indices.begin(), indices.end());
}

std::vector<ECS::EntityID> CollisionSpatialGrid::GetNearbyEntities(const Collision &target) const
{
    Vector3 min = target.GetMin();
    Vector3 max = target.GetMax();

    int minX = static_cast<int>(floorf(min.x / m_cellSize));
    int maxX = static_cast<int>(floorf(max.x / m_cellSize));
    int minZ = static_cast<int>(floorf(min.z / m_cellSize));
    int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

    std::unordered_set<ECS::EntityID> entities;
    for (int x = minX; x <= maxX; ++x)
    {
        for (int z = minZ; z <= maxZ; ++z)
        {
            GridKey key = {x, z};
            auto it = m_entityGrid.find(key);
            if (it != m_entityGrid.end())
            {
                for (auto entity : it->second)
                {
                    entities.insert(entity);
                }
            }
        }
    }

    return std::vector<ECS::EntityID>(entities.begin(), entities.end());
}

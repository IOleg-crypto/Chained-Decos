#ifndef COLLISIONSPATIALGRID_H
#define COLLISIONSPATIALGRID_H

#include "components/physics/collision/colsystem/CollisionSystem.h"
#include "components/physics/collision/structures/CollisionStructures.h"
#include "scene/ecs/Entity.h"
#include <memory>
#include <unordered_map>
#include <vector>


class CollisionSpatialGrid
{
public:
    explicit CollisionSpatialGrid(float cellSize = 10.0f);

    void Clear();
    void Build(const std::vector<std::shared_ptr<Collision>> &objects);
    void BuildEntities(
        const std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> &entityColliders);

    std::vector<size_t> GetNearbyObjectIndices(const Collision &target) const;
    std::vector<ECS::EntityID> GetNearbyEntities(const Collision &target) const;

    float GetCellSize() const
    {
        return m_cellSize;
    }
    void SetCellSize(float size)
    {
        m_cellSize = size;
    }

private:
    float m_cellSize;
    std::unordered_map<GridKey, std::vector<size_t>, GridKeyHash> m_staticGrid;
    std::unordered_map<GridKey, std::vector<ECS::EntityID>, GridKeyHash> m_entityGrid;
};

#endif // COLLISIONSPATIALGRID_H

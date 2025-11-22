#ifndef ICOLLISION_MANAGER_H
#define ICOLLISION_MANAGER_H

#include <vector>
#include <memory>
#include <raylib.h>

class Collision;

class ICollisionManager
{
public:
    virtual ~ICollisionManager() = default;
    
    virtual void AddCollider(Collision&& collider) = 0;
    virtual void ClearColliders() = 0;
    virtual const std::vector<std::unique_ptr<Collision>>& GetColliders() const = 0;
    virtual bool CheckCollision(const Collision& playerCollision) const = 0;
};

#endif // ICOLLISION_MANAGER_H

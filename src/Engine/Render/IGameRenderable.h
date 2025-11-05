#ifndef IGAMERENDERABLE_H
#define IGAMERENDERABLE_H

#include <raylib.h>
#include "Model/Model.h"
#include "Collision/CollisionManager.h"
#include "Collision/CollisionSystem.h"

// Інтерфейс для об'єктів, що потребують повного рендерингу гри (Player, NPC, тощо)
// Дотримується Interface Segregation Principle - тільки методи, потрібні для ігрових об'єктів
struct IGameRenderable
{
    virtual ~IGameRenderable() = default;

    // Оновлення стану об'єкта
    virtual void Update(CollisionManager& collisionManager) = 0;
    
    // Методи для рендерингу
    virtual Vector3 GetPosition() const = 0;
    virtual BoundingBox GetBoundingBox() const = 0;
    virtual float GetRotationY() const = 0;
    virtual void UpdateCollision() = 0;
    virtual const Collision& GetCollision() const = 0;
    virtual Camera GetCamera() const = 0;
    virtual bool IsGrounded() const = 0;
};

#endif // IGAMERENDERABLE_H

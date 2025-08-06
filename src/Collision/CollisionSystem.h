//
// Created by I#Oleg
//

#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <raylib.h>
#include <raymath.h>
#include <vector>

//
// Collision - class that handle all collision system in game engine
//
class Collision
{
  private:
    Vector3 m_min; // Position of the collision object
    Vector3 m_max; //
  public:
    Collision() = default;
    // Constructor to initialize position and size
    Collision(const Vector3 &center, const Vector3 &size);
    ~Collision() = default;
    // Getters for position and size
    // These methods allow access to the position and size of the collision object
    Vector3 GetMin() const;
    Vector3 GetMax() const;
    // Methods that checks collision
    void Update(const Vector3 &center, const Vector3 &size);
    bool Intersects(const Collision &other) const;
    bool Contains(const Vector3 &point) const;
};

#endif // COLLISIONSYSTEM_H
//
// Created by I#Oleg
//

#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <raylib.h>
#include <raymath.h>
#include <vector>


class Collision
{
private:
    Vector3 m_position; // Position of the collision object
    Vector3 m_size;     // Size of the collision object (e.g., bounding box)
public:
    Collision() = default;
    // Constructor to initialize position and size
    Collision(Vector3 position , Vector3 size);
    ~Collision() = default;
    Collision(const Collision &other) = delete;
    Collision(Collision &&other) = delete;
    // Getters for position and size
    // These methods allow access to the position and size of the collision object
    Vector3 GetPosition() const;
    Vector3 GetSize() const;
};

#endif // COLLISIONSYSTEM_H
//
// Created by I#Oleg
//

#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <cfloat>
#include <raylib.h>
#include <raymath.h>
#include <vector>

//
// Collision
// Handles an axis-aligned bounding box (AABB) for collision detection.
// Provides methods to update bounding box and check collisions or containment.
//
class Collision
{
public:
    Collision() = default;

    // Initialize collision box by center position and size (half-extents)
    Collision(const Vector3 &center, const Vector3 &size);

    ~Collision() = default;

    // -------------------- Getters --------------------

    // Get minimum corner of bounding box
    Vector3 GetMin() const;

    // Get maximum corner of bounding box
    Vector3 GetMax() const;

    // -------------------- Update --------------------

    // Update bounding box position and size
    void Update(const Vector3 &center, const Vector3 &size);

    // -------------------- Collision Checks --------------------

    // Check if this collision box intersects with another
    bool Intersects(const Collision &other) const;

    // Check if this collision box contains a point
    bool Contains(const Vector3 &point) const;

    // -------------------- Model Collision --------------------
    void CalculateFromModel(Model *model);
    void CalculateFromModel(Model *model, const Matrix &transform);

    Vector3 GetCenter() const;
    Vector3 GetSize() const;

private:
    Vector3 m_min{}; // Minimum corner of AABB
    Vector3 m_max{}; // Maximum corner of AABB
    Mesh m_mesh{};   // Optional mesh for rendering (not used in this example)
};

#endif // COLLISIONSYSTEM_H

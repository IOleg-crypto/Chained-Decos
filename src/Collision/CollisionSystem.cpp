//
// Created by I#Oleg
//
#include <Collision/CollisionSystem.h>
#include <raylib.h>


Collision::Collision(Vector3 position, Vector3 size) : m_position(position), m_size(size)
{
}

Vector3 Collision::GetPosition() const
{
    return m_position;
}
Vector3 Collision::GetSize() const
{
    return m_size;
}

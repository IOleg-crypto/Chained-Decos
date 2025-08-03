//
// Created by I#Oleg
//

#ifndef PHYSICSDATA_H
#define PHYSICSDATA_H

struct PhysicsData {
    float m_gravity = 10.0f; // Used Earth gravity as default
    float m_velocityY = 0.0;
    bool m_isGrounded = true;
    float m_groundLevel = 5.f; // if lower , you see player under world
    float m_dt = 0; // FrameRate Time
};

#endif //PHYSICSDATA_H

//
// Created by I#Oleg
//

#ifndef PHYSICSDATA_H
#define PHYSICSDATA_H

struct PhysicsData {
    float gravity = 10.0f; // Used Earth gravity as default
    float velocityY = 0.0;
    bool m_isGrounded = true;
    float GroundLevel = 5.f; // if lower , you see player under world
    float dt = 0; // FrameRate Time
};

#endif //PHYSICSDATA_H

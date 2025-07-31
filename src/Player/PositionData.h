//
// Created by I#Oleg.
//

#ifndef POSITIONDATA_H
#define POSITIONDATA_H

#include <raymath.h>

struct PositionData {
    Vector3 m_playerCurrentPosition = Vector3Zero();
    Vector3 m_playerLastPosition = Vector3Zero();
    Vector3 m_playerVelocity = Vector3Zero();
};
#endif //POSITIONDATA_H

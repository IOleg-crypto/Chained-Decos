//
// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <raylib.h>

class Player {
private:
    Camera camera;
public:
    Player();
    [[nodiscard]] Camera getCamera() const;
    void Update();
};

#endif //CAMERA_H

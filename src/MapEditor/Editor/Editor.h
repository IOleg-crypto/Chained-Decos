//
// Created by I#Oleg.
//

#ifndef EDITOR_H
#define EDITOR_H

#include <imgui.h>
#include <raylib.h>

#include <CameraController/CameraController.h>


class Editor {
private:
     CameraController *cameraController;
public:
     Editor();
     ~Editor();
};



#endif //EDITOR_H

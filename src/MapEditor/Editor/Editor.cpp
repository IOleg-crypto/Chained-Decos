//
// Created by I#Oleg
//

#include "Editor.h"

Editor::Editor() : cameraController(new CameraController())
{

}

Editor::~Editor() {
    delete cameraController;
}

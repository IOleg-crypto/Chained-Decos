//
// Created by I#Oleg
//
#ifndef APPLICATION_H
#define APPLICATION_H

#include <raylib.h>
#include <string>
#include <CameraController/CameraController.h>

// Main class for map editor
class Application {
public:
    Application(int width, int height);
    ~Application();
    void Init();
    void Run();
private:
    int m_width;
    int m_height;
    std::string m_WindowName;
    CameraController *cameraController;
};

#endif // APPLICATION_H 
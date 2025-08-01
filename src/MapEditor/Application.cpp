//
// Created by I#Oleg.
//
#include <MapEditor/Application.h>

Application::Application(int width, int height)
    : m_width(width), m_height(height) , m_WindowName("ChainedEditor") , cameraController(new CameraController()) {}

Application::~Application() {
    delete cameraController;
    CloseWindow();
}

void Application::Init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_width, m_height, m_WindowName.c_str());
    SetTargetFPS(60); 
}

void Application::Run() {
      while(!WindowShouldClose())
      {
          BeginDrawing();
          ClearBackground(BLUE);
          BeginMode3D(cameraController->getCamera());
          cameraController->SetCameraMode(CAMERA_FREE);
          cameraController->Update();
          DrawGrid(50, 0.1f);
          EndMode3D();
          EndDrawing(); 
      }
  }

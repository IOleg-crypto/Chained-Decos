//
// Created by I#Oleg.
//
#include <MapEditor/Application.h>

Application::Application(int width, int height)
    : m_width(width), m_height(height) , m_WindowName("ChainedEditor") , m_editor(std::make_unique<Editor>()) {}

Application::~Application() {
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
          BeginMode3D(m_editor->GetCameraController()->getCamera());
          m_editor->GetCameraController()->SetCameraMode(CAMERA_FREE);
          m_editor->GetCameraController()->Update();
          DrawGrid(50, 0.1f);
          EndMode3D();
          EndDrawing(); 
      }
  }

//
// Created by I#Oleg
//
#ifndef APPLICATION_H
#define APPLICATION_H

#include <raylib.h>
#include <string>
#include <memory>

#include "Editor/Editor.h"


// Main class for map editor
class Application {
public:
    Application(int width, int height);
    ~Application();
    Application(const Application&other) = delete;
    Application(Application &&other) = delete;
public:
    void Init();
    void Run();
private:
    int m_width;
    int m_height;
    std::string m_WindowName;
    std::unique_ptr<Editor> m_editor;
};

#endif // APPLICATION_H 
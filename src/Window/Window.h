//
// Created by I#Oleg on 20.07.2025.
//

#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <memory>
#include <raylib.h>

/**
 *  @brief Window - class that creates window using Raylib library
 */
class Window {
private:
      // Needed for screen resolution
      int m_screenX;
      int m_screenY;
      // Window name
      std::string m_WindowName;
public:
     Window();
     Window(const int screenX , const int screenY , const std::string &windowName);
     ~Window();
     // Init window from start
     void Init() const;
     // Run game
     void Run() const;
};
#endif //WINDOW_H

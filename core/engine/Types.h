#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

#include <cstdint>
#include <raylib.h>
#include <string>

namespace Core
{

// Window configuration
struct WindowConfig
{
    std::string Title = "Chained Decos Engine";
    int Width = 1280;
    int Height = 720;
    bool Fullscreen = false;
    bool VSync = true;
};

// Engine configuration
struct EngineConfig
{
    WindowConfig Window;
    bool EnableAudio = true;
    bool EnableDebug = false;
};

} // namespace Core

#endif // ENGINE_TYPES_H

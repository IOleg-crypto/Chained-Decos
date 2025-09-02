//
// Engine.cpp - Main Engine Implementation
//

#include "Engine.h"

// Standard library
#include <set>

// Raylib & ImGui
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlImGui.h>

Engine::Engine() : Engine(800, 600) {}

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"),
      m_windowInitialized(false), m_shouldExit(false), m_showDebug(false),
      m_showCollisionDebug(false), m_isEngineInit(false)
{
    constexpr int DEFAULT_SCREEN_WIDTH = 800;
    constexpr int DEFAULT_SCREEN_HEIGHT = 600;

    if (m_screenX <= 0 || m_screenY <= 0)
    {
        TraceLog(LOG_WARNING, "[Screen] Invalid screen size: %d x %d. Setting default size %dx%d.",
                 m_screenX, m_screenY, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
        m_screenX = DEFAULT_SCREEN_WIDTH;
        m_screenY = DEFAULT_SCREEN_HEIGHT;
    }

    TraceLog(LOG_INFO, "Engine initialized with screen size: %dx%d", m_screenX, m_screenY);
}

Engine::~Engine()
{
    TraceLog(LOG_INFO, "Starting Engine destructor...");

    if (m_windowInitialized && IsWindowReady())
    {
        TraceLog(LOG_INFO, "Closing window...");
        CloseWindow();
    }

    TraceLog(LOG_INFO, "Engine destructor completed");
}

void Engine::Init()
{
    TraceLog(LOG_INFO, "Initializing Engine...");

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    m_windowInitialized = true;
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    m_renderManager.Initialize();

    // Register engine-level input actions
    m_inputManager.RegisterAction(KEY_F11, ToggleFullscreen);

    m_isEngineInit = true;

    TraceLog(LOG_INFO, "Engine initialization complete!");
}

void Engine::Update()
{
    HandleEngineInput();
    m_inputManager.ProcessInput();
}

void Engine::Render()
{
    m_renderManager.BeginFrame();
    ClearBackground(RAYWHITE);
    m_renderManager.EndFrame();
}

bool Engine::ShouldClose() const { return WindowShouldClose() || m_shouldExit; }

void Engine::Shutdown() const
{
    TraceLog(LOG_INFO, "Shutting down Engine...");
    // m_renderManager.Shutdown();
    if (m_windowInitialized && IsWindowReady())
    {
        CloseWindow();
    }
    TraceLog(LOG_INFO, "Engine shutdown complete!");
}

void Engine::RequestExit()
{
    m_shouldExit = true;
    TraceLog(LOG_INFO, "Exit requested");
}

bool Engine::IsDebugInfoVisible() const { return m_showDebug; }

bool Engine::IsCollisionDebugVisible() const { return m_showCollisionDebug; }

// ==================== Private Engine Input Handling ====================
void Engine::HandleEngineInput()
{
    // Example: Toggle debug info (Game class can also register actions for F2/F3)
    if (IsKeyPressed(KEY_F2))
    {
        m_showDebug = !m_showDebug;
        TraceLog(LOG_INFO, "Debug info: %s", m_showDebug ? "ON" : "OFF");
    }
    if (IsKeyPressed(KEY_F3))
    {
        m_showCollisionDebug = !m_showCollisionDebug;
        TraceLog(LOG_INFO, "Collision debug: %s", m_showCollisionDebug ? "ON" : "OFF");
    }
}

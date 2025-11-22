//
// Engine.cpp - Main Engine Implementation
//

#include "Engine.h"
#include "Kernel/Core/Kernel.h"
#include "Module/Core/ModuleManager.h"
#include "Module/Interfaces/IEngineModule.h"
#include "Render/Core/RenderManager.h"
// Raylib & ImGui
#include <raylib.h>
#include <rlImGui.h>
#if defined(PLATFORM_DESKTOP)
#include "GLFW/glfw3.h"
#endif

Engine::Engine(const EngineConfig &config)
    : m_screenX(config.screenWidth), m_screenY(config.screenHeight), m_windowName("Chained Decos"),
      m_windowInitialized(false), m_renderManager(config.renderManager),
      m_inputManager(config.inputManager), m_kernel(config.kernel),
      m_moduleManager(std::make_unique<ModuleManager>(config.kernel)), m_shouldExit(false),
      m_isEngineInit(false)
{
    if (m_screenX <= 0 || m_screenY <= 0)
    {
        constexpr int DEFAULT_SCREEN_WIDTH = 1280;
        constexpr int DEFAULT_SCREEN_HEIGHT = 720;
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
        rlImGuiShutdown();
        CloseWindow();
    }

    TraceLog(LOG_INFO, "Engine destructor completed");
}

void Engine::Init()
{
    TraceLog(LOG_INFO, "Initializing Engine...");

    // FLAG_MOUSE_CAPTURE_ALWAYS helps with mouse input on virtual machines
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    m_windowInitialized = true;
    SetExitKey(KEY_NULL);

    // SetMouseGrabbed helps with mouse sensitivity on virtual machines
    // Note: This can be controlled dynamically via SetMouseGrabbed(false) when menu is open

    // Center window on screen
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    int windowX = (monitorWidth - m_screenX) / 2;
    int windowY = (monitorHeight - m_screenY) / 2;
    SetWindowPosition(windowX, windowY);

    // Initialize raw mouse motion for better mouse handling on Linux/VM
    // This must be done after window is created and before other input handling
#if defined(__linux__) // Crashing on Windows
    if (glfwRawMouseMotionSupported())
    {
        GLFWwindow *window = (GLFWwindow *)GetWindowHandle();
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        TraceLog(LOG_INFO, "[Engine] Raw mouse motion enabled (Linux/VM)");
    }
    else
    {
        TraceLog(LOG_INFO, "[Engine] Raw mouse motion not supported on this system");
    }
#endif

    rlImGuiSetup(true);
    // m_inputManager->RegisterAction(KEY_F11, ToggleFullscreen);
    m_isEngineInit = true;

    if (m_kernel)
    {
        m_kernel->RegisterService<RenderManager>(m_renderManager);
        m_kernel->RegisterService<InputManager>(m_inputManager);
    }

    TraceLog(LOG_INFO, "Engine initialization complete!");
}

void Engine::Update()
{
    if (m_kernel)
    {
        m_kernel->Update(GetFrameTime());
    }

    // Modules are updated via Engine::Update which is called from Game::Update
    // Game::Update checks m_isGameInitialized before calling UpdatePlayerLogic,
    // but modules are still updated. PlayerModule::Update() now checks player position.
    if (m_moduleManager)
    {
        m_moduleManager->UpdateAllModules(GetFrameTime());
    }

    HandleEngineInput();
}

void Engine::Render() const
{
    // Note: BeginFrame/EndFrame are now called in EngineApplication::Render()
    // to allow projects to render between them

    if (m_moduleManager)
    {
        m_moduleManager->RenderAllModules();
    }
}

bool Engine::ShouldClose() const
{
    return WindowShouldClose() || m_shouldExit;
}

void Engine::Shutdown() const
{
    TraceLog(LOG_INFO, "Shutting down Engine...");
    if (m_windowInitialized && IsWindowReady())
    {
        TraceLog(LOG_INFO, "Closing window...");
        rlImGuiShutdown();
        CloseWindow();
    }
    TraceLog(LOG_INFO, "Engine shutdown complete!");
}

void Engine::RequestExit()
{
    m_shouldExit = true;
    TraceLog(LOG_INFO, "Exit requested");
}

void Engine::SetWindowName(const std::string &name)
{
    m_windowName = name;
    if (m_windowInitialized && IsWindowReady())
    {
        SetWindowTitle(name.c_str());
    }
}

bool Engine::IsDebugInfoVisible() const
{
    return m_renderManager ? m_renderManager->IsDebugInfoVisible() : false;
}

bool Engine::IsCollisionDebugVisible() const
{
    return m_renderManager ? m_renderManager->IsCollisionDebugVisible() : false;
}

void Engine::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (m_moduleManager && module)
    {
        m_moduleManager->RegisterModule(std::move(module));
    }
}

void Engine::HandleEngineInput()
{
    if (IsKeyPressed(KEY_F2))
    {
        if (m_renderManager)
        {
            m_renderManager->ToggleDebugInfo();
            TraceLog(LOG_INFO, "Debug info: %s",
                     m_renderManager->IsDebugInfoVisible() ? "ON" : "OFF");
        }
    }
    if (IsKeyPressed(KEY_F3))
    {
        if (m_renderManager)
        {
            m_renderManager->ToggleCollisionDebug();
            TraceLog(LOG_INFO, "Collision debug: %s",
                     m_renderManager->IsCollisionDebugVisible() ? "ON" : "OFF");
        }
    }
}
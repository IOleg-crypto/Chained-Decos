//
// Engine.cpp - Main Engine Implementation
//

#include "Engine.h"
#include "Render/RenderManager.h"
#include "Kernel/Kernel.h"
// Raylib & ImGui
#include <raylib.h>
#include <rlImGui.h>

Engine::Engine() : Engine(800, 600) {}

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"),
      m_windowInitialized(false), m_renderManager(std::make_shared<RenderManager>()), m_shouldExit(false),
      m_showDebug(false), m_showCollisionDebug(true), m_isEngineInit(false)
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    m_windowInitialized = true;
    SetExitKey(KEY_NULL);

    m_renderManager->Initialize();
    m_renderManager->SetCollisionDebug(m_showCollisionDebug);

    // Register engine-level input actions
    m_inputManager.RegisterAction(KEY_F11, ToggleFullscreen);

    m_isEngineInit = true;

    // Kernel: register core services so other modules can fetch them
    Kernel &kernel = Kernel::GetInstance();
    // Wrap RenderManager with a small IKernelService adapter to satisfy interface without changing RenderManager
    struct RenderServiceAdapter : public IKernelService {
        std::shared_ptr<RenderManager> rm;
        explicit RenderServiceAdapter(std::shared_ptr<RenderManager> r) : rm(std::move(r)) {}
        bool Initialize() override { return rm != nullptr; }
        void Shutdown() override {}
        void Render() override { /* optional: could call debug hooks */ }
        const char *GetName() const override { return "RenderServiceAdapter"; }
    };
    kernel.RegisterService<RenderServiceAdapter>(Kernel::ServiceType::Render, std::make_shared<RenderServiceAdapter>(m_renderManager));

    TraceLog(LOG_INFO, "Engine initialization complete!");
}

void Engine::Update()
{
    HandleEngineInput();
    m_inputManager.ProcessInput();
}

void Engine::Render() const
{
    m_renderManager->BeginFrame();
    ClearBackground(RAYWHITE);
    m_renderManager->EndFrame();
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

RenderManager *Engine::GetRenderManager() const { return m_renderManager.get(); }

InputManager &Engine::GetInputManager() { return m_inputManager; }

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
    if (IsKeyPressed(KEY_F2))
    {
        m_showDebug = !m_showDebug;
        m_renderManager->SetDebugInfo(m_showDebug);
        TraceLog(LOG_INFO, "Debug info: %s", m_showDebug ? "ON" : "OFF");
    }
    if (IsKeyPressed(KEY_F3))
    {
        m_showCollisionDebug = !m_showCollisionDebug;
        m_renderManager->SetCollisionDebug(m_showCollisionDebug);
        TraceLog(LOG_INFO, "Collision debug: %s", m_showCollisionDebug ? "ON" : "OFF");
    }
}
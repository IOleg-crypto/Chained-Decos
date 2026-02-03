#ifndef CH_APPLICATION_H
#define CH_APPLICATION_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/core/layer.h"
#include "engine/core/layer_stack.h"
#include "engine/core/window.h"
#include "engine/core/timestep.h"
#include "engine/scene/scene.h"
#include "raylib.h"
#include <string>
#include <memory> // Added for std::unique_ptr

namespace CHEngine
{
    /**
     * Configuration settings for the Application.
     * Inherits window properties and adds application-specific arguments and icons.
     */
    struct ApplicationConfig : public WindowProps
    {
        Image WindowIcon = {0};
        std::string StartScene;
        int Argc = 0;
        char **Argv = nullptr;

        ApplicationConfig() = default;
    };

    /**
     * The main entry point and controller for the engine life cycle.
     * Manages windowing, layers, events, and the main execution loop.
     * Follows the Singleton pattern for global engine access.
     */
    class Application
    {
    public:
        using Config = ApplicationConfig;

    public: // Life Cycle
        Application(const Config &config = Config());
        virtual ~Application();

        /** Initializes core engine systems and creating the window. */
        bool Initialize(const Config &config);
        
        /** Gracefully shuts down all systems and cleans up resources. */
        void Shutdown();
        
        /** Signal the application to close on the next frame. */
        void Close();

    public: // Layer Management
        static void PushLayer(Layer *layer);
        static void PushOverlay(Layer *overlay);

    public: // Event and Frame Control
        static void OnEvent(Event &e);
        static bool ShouldClose();
        
        /** Prepares the engine for a new frame (Clears buffers, starts ImGui). */
        static void BeginFrame();
        
        /** Finalizes the frame (Blits to screen, ends ImGui). */
        static void EndFrame();

    public: // Getters & Setters
        inline static Application &Get() { return *s_Instance; }

        inline LayerStack &GetLayerStack() { return m_LayerStack; }
        inline std::unique_ptr<Window> &GetWindow() { return m_Window; }
        inline const Config &GetConfig() const { return m_Config; }
        inline static Timestep GetDeltaTime() { return s_Instance->m_DeltaTime; }
        inline static bool IsRunning() { return s_Instance->m_Running; }

        void SetWindowIcon(const Image &icon) const;

    public: // Template hooks for client apps
        virtual void PostInitialize() {}

    private: // Internal Processing
        void Run();
        void ProcessEvents();
        void Simulate();
        void Render();

        /** Internal helper to load engine-specific fonts and icons. */
        void LoadEngineFonts();

    private: // Global State
        static Application *s_Instance;

    private: // Members
        Config m_Config;
        bool m_Running = false;
        bool m_Minimized = false;
        
        Timestep m_DeltaTime = 0.0f;
        float m_LastFrameTime = 0.0f;

        LayerStack m_LayerStack;
        std::unique_ptr<Window> m_Window;

        // Allowing main to call Run()
        friend int ::main(int argc, char** argv);
        friend Application* CreateApplication(int argc, char** argv);
    };

    /** To be defined in the client application. */
    Application *CreateApplication(int argc, char **argv);

} // namespace CHEngine

#endif // CH_APPLICATION_H

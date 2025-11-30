#include "CoreServices.h"
#include <raylib.h>

bool CoreServices::Initialize(int width, int height, const char *title, bool fullscreen, bool vsync)
{
    TraceLog(LOG_INFO, "[CoreServices] Initializing Servers...");

    // 1. RenderingServer (Creates Window & Context)
    render = std::make_shared<RenderManager>();
    if (!render->Initialize(width, height, title, fullscreen, vsync))
    {
        TraceLog(LOG_ERROR, "[CoreServices] Failed to initialize RenderingServer");
        return false;
    }
    TraceLog(LOG_INFO, "[CoreServices] RenderingServer initialized");

    // 2. InputServer (Depends on Window)
    input = std::make_shared<InputManager>();
    if (!input->Initialize())
    {
        TraceLog(LOG_ERROR, "[CoreServices] Failed to initialize InputServer");
        return false;
    }
    TraceLog(LOG_INFO, "[CoreServices] InputServer initialized");

    // 3. AudioServer
    audio = std::make_shared<AudioManager>();
    if (!audio->Initialize())
    {
        TraceLog(LOG_ERROR, "[CoreServices] Failed to initialize AudioServer");
        return false;
    }
    TraceLog(LOG_INFO, "[CoreServices] AudioServer initialized");

    // 4. PhysicsServer
    physics = std::make_shared<CollisionManager>();
    TraceLog(LOG_INFO, "[CoreServices] PhysicsServer initialized");

    // 5. ResourceServer (Model)
    resources = std::make_shared<Model>();
    // Note: Model might need initialization if it has specific setup,
    // but based on previous code it was just make_shared.
    // If Model has an Initialize method, call it here.
    if (resources->Initialize())
    {
        TraceLog(LOG_INFO, "[CoreServices] ResourceServer (Model) initialized");
    }
    else
    {
        TraceLog(
            LOG_WARNING,
            "[CoreServices] ResourceServer (Model) initialization returned false (or not needed)");
    }

    // 6. WorldServer
    world = std::make_shared<WorldManager>();
    TraceLog(LOG_INFO, "[CoreServices] WorldServer initialized");

    TraceLog(LOG_INFO, "[CoreServices] All Servers initialized successfully");
    return true;
}

void CoreServices::Shutdown()
{
    TraceLog(LOG_INFO, "[CoreServices] Shutting down Servers...");

    // Shutdown in reverse order

    world.reset();
    TraceLog(LOG_INFO, "[CoreServices] WorldServer shutdown");

    if (resources)
    {
        resources->Shutdown();
        resources.reset();
        TraceLog(LOG_INFO, "[CoreServices] ResourceServer shutdown");
    }

    physics.reset();
    TraceLog(LOG_INFO, "[CoreServices] PhysicsServer shutdown");

    if (audio)
    {
        audio->Shutdown();
        audio.reset();
        TraceLog(LOG_INFO, "[CoreServices] AudioServer shutdown");
    }

    if (input)
    {
        input->Shutdown();
        input.reset();
        TraceLog(LOG_INFO, "[CoreServices] InputServer shutdown");
    }

    if (render)
    {
        render->Shutdown();
        render.reset();
        TraceLog(LOG_INFO, "[CoreServices] RenderingServer shutdown");
    }

    TraceLog(LOG_INFO, "[CoreServices] Shutdown complete");
}

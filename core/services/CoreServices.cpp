#include "CoreServices.h"
#include "core/engine/Engine.h"
#include <raylib.h>

bool CoreServices::Initialize(int width, int height, const char *title, bool fullscreen, bool vsync)
{
    // 1. RenderingServer (Managed via singleton)
    render = std::shared_ptr<RenderManager>(&RenderManager::Get(), [](RenderManager *) {});
    Engine::Instance().RegisterService<RenderManager>(render);

    // 2. InputServer (Managed via singleton)
    input = std::shared_ptr<InputManager>(&InputManager::Get(), [](InputManager *) {});
    Engine::Instance().RegisterService<InputManager>(input);

    // 3. AudioServer (Managed via singleton)
    audio = std::shared_ptr<AudioManager>(&AudioManager::Get(), [](AudioManager *) {});
    Engine::Instance().RegisterService<AudioManager>(audio);

    // 4. PhysicsServer
    physics = std::make_shared<CollisionManager>();
    Engine::Instance().RegisterService<CollisionManager>(physics);

    // 5. ResourceServer (Model)
    resources = std::make_shared<ModelLoader>();
    Engine::Instance().RegisterService<ModelLoader>(resources);

    // 6. LevelServer is registered as a Module in GameApplication
    // and will register itself as a service during its Initialize/RegisterServices phase.

    // 7. WorldServer
    world = std::make_shared<WorldManager>();
    Engine::Instance().RegisterService<WorldManager>(world);

    return true;
}

void CoreServices::Shutdown()
{
    // Shutdown in reverse order
    world.reset();
    if (resources)
    {
        resources.reset();
    }
    physics.reset();
    if (audio)
    {
        audio->Shutdown();
        audio.reset();
    }
    if (input)
    {
        input->Shutdown();
        input.reset();
    }
    if (render)
    {
        render->Shutdown();
        render.reset();
    }
}

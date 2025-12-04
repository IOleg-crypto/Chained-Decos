#ifndef CORE_SERVICES_H
#define CORE_SERVICES_H

#include "scene/main/Core/World.h"
#include "scene/resources/model/Core/Model.h"
#include "components/audio/Core/AudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/physics/collision/Core/CollisionManager.h"
#include "components/rendering/Core/RenderManager.h"
#include <memory>


// CoreServices acts as the Registry of Servers (Godot-style)
// These are low-level systems that don't know about game logic
struct CoreServices
{
    // Servers
    std::shared_ptr<RenderManager> render;     // RenderingServer
    std::shared_ptr<InputManager> input;       // InputServer
    std::shared_ptr<AudioManager> audio;       // AudioServer
    std::shared_ptr<CollisionManager> physics; // PhysicsServer
    std::shared_ptr<Model> resources;          // ResourceServer (ModelLoader)
    std::shared_ptr<WorldManager> world;       // SceneServer/WorldServer

    // Initialize all servers in the correct order
    bool Initialize(int width, int height, const char *title, bool fullscreen, bool vsync);

    // Shutdown all servers in reverse order
    void Shutdown();
};

#endif // CORE_SERVICES_H

#include "engine_layer.h"
#include "core/log.h"
#include "engine/audio/audio.h"
#include "engine/gui/gui_manager.h"
#include "engine/physics/collision/core/collision_manager.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/core/scene_manager.h"
#include "engine/scene/resources/font/font_service.h"
#include "engine/scene/resources/model/model.h"
#include "engine/scene/resources/texture/texture_service.h"


namespace CHEngine
{
EngineLayer::EngineLayer() : Layer("EngineLayer")
{
}

void EngineLayer::OnAttach()
{
    CD_CORE_INFO("EngineLayer attached. Initializing simulation systems...");

    // Moved from Application::Application
    CollisionManager::Init();
    ModelLoader::Init();
    Audio::Init();
    SceneManager::Init();
    GuiManager::Init();
    FontService::Init();
    TextureService::Init();
}

void EngineLayer::OnDetach()
{
    CD_CORE_INFO("EngineLayer detached. Shutting down simulation systems...");

    // Moved from Application::~Application
    TextureService::Shutdown();
    FontService::Shutdown();
    SceneManager::Shutdown();
    GuiManager::Shutdown();
    Audio::Shutdown();
    ModelLoader::Shutdown();
    CollisionManager::Shutdown();
}

void EngineLayer::OnUpdate(float deltaTime)
{
    // Engine-level updates
    Audio::Update(deltaTime);
    SceneManager::Update(deltaTime);
}

void EngineLayer::OnEvent(Event &event)
{
    // Engine-level event handling if needed
}
} // namespace CHEngine

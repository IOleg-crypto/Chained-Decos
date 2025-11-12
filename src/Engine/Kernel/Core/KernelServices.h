#ifndef KERNELSERVICES_H
#define KERNELSERVICES_H

#include "../Interfaces/IKernelService.h"
#include "../Input/Core/InputManager.h"
#include "../Collision/Manager/CollisionManager.h"
#include "../Model/Core/Model.h"
#include "../World/Core/World.h"
#include "../Audio/Core/AudioManager.h"
#include "../Event/Core/EventSystem.h"
#include "../Asset/Core/AssetManager.h"
#include "../Render/Manager/RenderManager.h"
#include "../../Game/Player/Interfaces/IPlayerProvider.h"
#include "../../Game/Managers/IMapManagerProvider.h"
#include "../IEngineProvider.h"

struct RenderService : public IKernelService
{
    RenderManager *renderManager = nullptr;
    explicit RenderService(RenderManager *rm) : renderManager(rm) {}
    bool Initialize() override { return renderManager != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "RenderService"; }
};

struct InputService : public IKernelService
{
    InputManager *input = nullptr;
    explicit InputService(InputManager *mgr) : input(mgr) {}
    bool Initialize() override { return input != nullptr; }
    void Shutdown() override {}
    void Update(float) override { if (input) input->ProcessInput(); }
    const char *GetName() const override { return "InputService"; }
};

struct CollisionService : public IKernelService
{
    CollisionManager *cm = nullptr;
    explicit CollisionService(CollisionManager *m) : cm(m) {}
    bool Initialize() override { return cm != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "CollisionService"; }
};

struct ModelsService : public IKernelService
{
    ModelLoader *models = nullptr;
    explicit ModelsService(ModelLoader *m) : models(m) {}
    bool Initialize() override { return models != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "ModelsService"; }
};

struct WorldService : public IKernelService
{
    WorldManager *world = nullptr;
    explicit WorldService(WorldManager *w) : world(w) {}
    bool Initialize() override { return world != nullptr; }
    void Shutdown() override {}
    void Update(float dt) override { if (world) world->Update(dt); }
    const char *GetName() const override { return "WorldService"; }
};

struct AudioService : public IKernelService
{
    AudioManager *audio = nullptr;
    explicit AudioService(AudioManager *a) : audio(a) {}
    bool Initialize() override { return audio != nullptr && audio->Initialize(); }
    void Shutdown() override { if (audio) audio->UnloadAll(); }
    const char *GetName() const override { return "AudioService"; }
};

struct AssetService : public IKernelService
{
    AssetManager *assets = nullptr;
    explicit AssetService(AssetManager *a) : assets(a) {}
    bool Initialize() override { return assets != nullptr; }
    void Shutdown() override { if (assets) assets->UnloadAll(); }
    const char *GetName() const override { return "AssetService"; }
};

// Forward declarations for game services
class Player;
class Menu;
class MapManager;
class ResourceManager;
class PlayerManager;
class StateManager;
class Engine;

struct PlayerService : public IKernelService, public IPlayerProvider
{
    Player *player = nullptr;
    explicit PlayerService(Player *p) : player(p) {}
    bool Initialize() override { return player != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "PlayerService"; }
    Player* GetPlayer() override { return player; }
};

struct MenuService : public IKernelService
{
    Menu *menu = nullptr;
    explicit MenuService(Menu *m) : menu(m) {}
    bool Initialize() override { return menu != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "MenuService"; }
};

struct MapManagerService : public IKernelService, public IMapManagerProvider
{
    MapManager *mapManager = nullptr;
    explicit MapManagerService(MapManager *mm) : mapManager(mm) {}
    bool Initialize() override { return mapManager != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "MapManagerService"; }
    MapManager* GetMapManager() override { return mapManager; }
};

struct ResourceManagerService : public IKernelService
{
    ResourceManager *resourceManager = nullptr;
    explicit ResourceManagerService(ResourceManager *rm) : resourceManager(rm) {}
    bool Initialize() override { return resourceManager != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "ResourceManagerService"; }
};

struct PlayerManagerService : public IKernelService
{
    PlayerManager *playerManager = nullptr;
    explicit PlayerManagerService(PlayerManager *pm) : playerManager(pm) {}
    bool Initialize() override { return playerManager != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "PlayerManagerService"; }
};

struct StateManagerService : public IKernelService
{
    StateManager *stateManager = nullptr;
    explicit StateManagerService(StateManager *sm) : stateManager(sm) {}
    bool Initialize() override { return stateManager != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "StateManagerService"; }
};

struct EngineService : public IKernelService, public IEngineProvider
{
    Engine *engine = nullptr;
    explicit EngineService(Engine *e) : engine(e) {}
    bool Initialize() override { return engine != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "EngineService"; }
    Engine* GetEngine() override { return engine; }
};

#endif // KERNELSERVICES_H



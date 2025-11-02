#ifndef KERNELSERVICES_H
#define KERNELSERVICES_H

#include "IKernelService.h"
#include "../Input/InputManager.h"
#include "../Collision/CollisionManager.h"
#include "../Model/Model.h"
#include "../World/World.h"
#include "../Audio/AudioManager.h"
#include "../Event/EventSystem.h"
#include "../Asset/AssetManager.h"

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
class Game;

struct PlayerService : public IKernelService
{
    Player *player = nullptr;
    explicit PlayerService(Player *p) : player(p) {}
    bool Initialize() override { return player != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "PlayerService"; }
};

struct MenuService : public IKernelService
{
    Menu *menu = nullptr;
    explicit MenuService(Menu *m) : menu(m) {}
    bool Initialize() override { return menu != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "MenuService"; }
};

struct MapManagerService : public IKernelService
{
    MapManager *mapManager = nullptr;
    explicit MapManagerService(MapManager *mm) : mapManager(mm) {}
    bool Initialize() override { return mapManager != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "MapManagerService"; }
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

struct GameService : public IKernelService
{
    Game *game = nullptr;
    explicit GameService(Game *g) : game(g) {}
    bool Initialize() override { return game != nullptr; }
    void Shutdown() override {}
    const char *GetName() const override { return "GameService"; }
};

#endif // KERNELSERVICES_H



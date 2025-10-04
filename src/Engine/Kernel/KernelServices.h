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

#endif // KERNELSERVICES_H



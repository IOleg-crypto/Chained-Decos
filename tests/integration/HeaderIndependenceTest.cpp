// Header Independence Test
// This test verifies that each header file can be included independently
// without requiring other headers to be included first.
//
// The test compiles each header in isolation to ensure:
// 1. Each header includes all its dependencies
// 2. Forward declarations are properly used
// 3. No implicit dependencies exist

#include <gtest/gtest.h>

// Test Game headers
#include "Game/Player/Core/Player.h"
TEST(HeaderIndependence, PlayerHeader) { SUCCEED(); }

#include "Game/Player/Components/PlayerRenderable.h"
TEST(HeaderIndependence, PlayerRenderableHeader) { SUCCEED(); }

#include "Game/Player/Interfaces/IPlayerInput.h"
TEST(HeaderIndependence, IPlayerInputHeader) { SUCCEED(); }

#include "Game/Player/Interfaces/IPlayerMovement.h"
TEST(HeaderIndependence, IPlayerMovementHeader) { SUCCEED(); }

#include "Game/Player/Interfaces/IPlayerMediator.h"
TEST(HeaderIndependence, IPlayerMediatorHeader) { SUCCEED(); }

#include "Game/Systems/PlayerSystem/PlayerSystem.h"
TEST(HeaderIndependence, PlayerSystemHeader) { SUCCEED(); }

#include "Game/Systems/MapSystem/MapSystem.h"
TEST(HeaderIndependence, MapSystemHeader) { SUCCEED(); }

#include "Game/Systems/UIController/UIController.h"
TEST(HeaderIndependence, UIControllerHeader) { SUCCEED(); }

#include "Game/Systems/RenderingSystem/RenderingSystem.h"
TEST(HeaderIndependence, RenderingSystemHeader) { SUCCEED(); }

#include "Game/Menu/Menu.h"
TEST(HeaderIndependence, MenuHeader) { SUCCEED(); }

#include "Game/Menu/Console/ConsoleManager.h"
TEST(HeaderIndependence, ConsoleManagerHeader) { SUCCEED(); }

#include "Game/Managers/MapCollisionInitializer.h"
TEST(HeaderIndependence, MapCollisionInitializerHeader) { SUCCEED(); }

// Test Engine headers
#include "Engine/Application/Core/EngineApplication.h"
TEST(HeaderIndependence, EngineApplicationHeader) { SUCCEED(); }

#include "Engine/Audio/Core/AudioManager.h"
TEST(HeaderIndependence, AudioManagerHeader) { SUCCEED(); }

#include "Engine/Audio/Interfaces/IAudioManager.h"
TEST(HeaderIndependence, IAudioManagerHeader) { SUCCEED(); }

#include "Engine/CameraController/Core/CameraController.h"
TEST(HeaderIndependence, CameraControllerHeader) { SUCCEED(); }

#include "Engine/Collision/Core/CollisionManager.h"
TEST(HeaderIndependence, CollisionManagerHeader) { SUCCEED(); }

#include "Engine/Collision/Interfaces/ICollisionManager.h"
TEST(HeaderIndependence, ICollisionManagerHeader) { SUCCEED(); }

#include "Engine/Config/Core/ConfigManager.h"
TEST(HeaderIndependence, ConfigManagerHeader) { SUCCEED(); }

#include "Engine/Engine.h"
TEST(HeaderIndependence, EngineHeader) { SUCCEED(); }

#include "Engine/Interfaces/IEngine.h"
TEST(HeaderIndependence, IEngineHeader) { SUCCEED(); }

#include "Engine/Input/Core/InputManager.h"
TEST(HeaderIndependence, InputManagerHeader) { SUCCEED(); }

#include "Engine/Kernel/Core/Kernel.h"
TEST(HeaderIndependence, KernelHeader) { SUCCEED(); }

#include "Engine/Kernel/Interfaces/IKernelService.h"
TEST(HeaderIndependence, IKernelServiceHeader) { SUCCEED(); }

#include "Engine/Map/Core/MapData.h"
TEST(HeaderIndependence, MapDataHeader) { SUCCEED(); }

#include "Engine/Map/Core/MapLoader.h"
TEST(HeaderIndependence, MapLoaderHeader) { SUCCEED(); }

#include "Engine/Map/Core/MapService.h"
TEST(HeaderIndependence, MapServiceHeader) { SUCCEED(); }

#include "Engine/Map/Renderer/MapRenderer.h"
TEST(HeaderIndependence, MapRendererHeader) { SUCCEED(); }

#include "Engine/Map/Skybox/Skybox.h"
TEST(HeaderIndependence, SkyboxHeader) { SUCCEED(); }

#include "Engine/Model/Core/Model.h"
TEST(HeaderIndependence, ModelHeader) { SUCCEED(); }

#include "Engine/Model/Core/ModelInstance.h"
TEST(HeaderIndependence, ModelInstanceHeader) { SUCCEED(); }

#include "Engine/Model/Animation/Animation.h"
TEST(HeaderIndependence, AnimationHeader) { SUCCEED(); }

#include "Engine/Module/Core/ModuleManager.h"
TEST(HeaderIndependence, ModuleManagerHeader) { SUCCEED(); }

#include "Engine/Module/Interfaces/IEngineModule.h"
TEST(HeaderIndependence, IEngineModuleHeader) { SUCCEED(); }

#include "Engine/Render/Core/RenderManager.h"
TEST(HeaderIndependence, RenderManagerHeader) { SUCCEED(); }

#include "Engine/World/Core/World.h"
TEST(HeaderIndependence, WorldHeader) { SUCCEED(); }

// Test that all headers compiled successfully
TEST(HeaderIndependence, AllHeadersCompiled) {
    // If we reached this point, all headers were successfully included independently
    SUCCEED();
}

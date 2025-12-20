#ifndef SERVERS_H
#define SERVERS_H

// Servers headers - central include file for all server types
// This eliminates the need for forward declarations
// Following Godot architecture principles

// Must include core first (dependency)
#include "core/utils/core.h"

// Rendering server
#include "rendering/commands/IRenderCommand.h"
#include "rendering/core/RenderManager.h"
#include "rendering/interfaces/IGameRenderable.h"
#include "rendering/interfaces/IRenderable.h"
#include "rendering/shader/shaderManager.h"

// Audio server
#include "audio/core/AudioManager.h"
#include "audio/interfaces/IAudioManager.h"

// Input server
#include "input/core/InputManager.h"

// Physics server
#include "physics/collision/core/collisionManager.h"
#include "physics/collision/debug/collisionDebugRenderer.h"
#include "physics/collision/interfaces/ICollisionManager.h"
#include "physics/collision/structures/collisionComponent.h"
#include "physics/collision/structures/collisionStructures.h"
#include "physics/dynamics/components/PhysicsComponent.h"
#include "physics/dynamics/components/SurfaceComponent.h"

#endif // SERVERS_H







#ifndef SERVERS_H
#define SERVERS_H

// Servers headers - central include file for all server types
// This eliminates the need for forward declarations
// Following Godot architecture principles

// Must include core first (dependency)
#include "core/utils/core.h"

// Rendering server
#include "rendering/Commands/IRenderCommand.h"
#include "rendering/Core/RenderManager.h"
#include "rendering/Interfaces/IGameRenderable.h"
#include "rendering/Interfaces/IRenderable.h"
#include "rendering/Shader/ShaderManager.h"

// Audio server
#include "audio/Core/AudioManager.h"
#include "audio/Interfaces/IAudioManager.h"

// Input server
#include "input/Core/InputManager.h"

// Physics server
#include "physics/collision/Core/CollisionManager.h"
#include "physics/collision/Debug/CollisionDebugRenderer.h"
#include "physics/collision/Interfaces/ICollisionManager.h"
#include "physics/collision/Structures/CollisionComponent.h"
#include "physics/collision/Structures/CollisionStructures.h"
#include "physics/dynamics/Components/PhysicsComponent.h"
#include "physics/dynamics/Components/SurfaceComponent.h"

#endif // SERVERS_H





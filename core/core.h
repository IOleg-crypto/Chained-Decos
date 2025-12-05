#ifndef CORE_H
#define CORE_H

// Core headers - central include file for all core types
// This eliminates the need for forward declarations
// Following Godot architecture principles

// Base interfaces
#include "object/kernel/Interfaces/IKernelService.h"
#include "object/module/Interfaces/IEngineModule.h"
#include "object/module/Interfaces/IModule.h"

// Core objects (in dependency order)
#include "object/kernel/Core/Kernel.h"
#include "object/module/Core/ModuleContext.h"
#include "object/module/Core/ModuleManager.h"

// Event system
#include "object/event/Core/Event.h"
#include "object/event/Core/EventDispatcher.h"
#include "object/event/Core/EventSystem.h"

#endif // CORE_H

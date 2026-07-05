#ifndef SCRIPT_GLUE_SCENE_H
#define SCRIPT_GLUE_SCENE_H
#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/core/log.h"
#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/app/application.h"
#include "engine/scene/scene_events.h"

namespace Chained
{
CH_SCRIPT_FUNC uint64_t Scene_FindEntityByTag(Coral::String tag);
CH_SCRIPT_FUNC uint64_t Scene_CopyEntity(uint64_t entityID);
CH_SCRIPT_FUNC void Scene_LoadScene(Coral::String path);
CH_SCRIPT_FUNC uint64_t Scene_GetPrimaryCameraEntity();

} // namespace Chained
#endif
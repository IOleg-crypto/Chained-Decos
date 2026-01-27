#ifndef CH_SCENE_SCRIPT_H
#define CH_SCENE_SCRIPT_H

#include "engine/core/base.h"
#include "engine/core/log.h"
#include "engine/scene/scriptable_entity.h"

namespace CHEngine
{
CH_SCRIPT(SceneScript){CH_START(){CH_CORE_INFO("SceneScript: OnStart running...");

}

CH_UPDATE(dt)
{
    CH_CORE_INFO("SceneScript: OnUpdate running...");
}
}
;
} // namespace CHEngine

#endif // CH_SCENE_SCRIPT_H
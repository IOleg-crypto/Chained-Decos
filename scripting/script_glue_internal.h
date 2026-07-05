#ifndef CH_SCRIPT_GLUE_INTERNAL_H
#define CH_SCRIPT_GLUE_INTERNAL_H

#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include "scriptengine.h"
#include "scriptengine_services.h"
#include <Coral/Array.hpp>
#include <Coral/String.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#define CH_SCRIPT_FUNC extern "C"

namespace Chained
{

extern void RegisterGlueSystem();
extern void RegisterGlueInput();
extern void RegisterGlueNetwork();
extern void RegisterGlueScene();
extern void RegisterGlueEntity();
extern void RegisterGlueCamera();
extern void RegisterGlueUI();
extern void RegisterGlueAudio();

extern Scene* GetContextScene(); 

extern Scene* GetActiveScene();

Entity GetEntity(uint64_t entityID);
} // namespace Chained

#endif // CH_SCRIPT_GLUE_INTERNAL_H
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

static inline std::string ch_u16_to_string(const char16_t* ptr)
{
    if (!ptr) return {};
    std::u16string u16(ptr);
    std::string result;
    result.reserve(u16.size());
    for (char16_t c : u16) result += (c < 0x80) ? (char)c : '?';
    return result;
}

static inline std::u16string ch_utf8_to_u16(const std::string& str)
{
    std::u16string result;
    result.reserve(str.size());
    for (char c : str) result += (char16_t)(unsigned char)c;
    return result;
}

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
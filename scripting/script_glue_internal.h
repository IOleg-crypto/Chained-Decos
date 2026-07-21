#ifndef CH_SCRIPT_GLUE_INTERNAL_H
#define CH_SCRIPT_GLUE_INTERNAL_H

#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include "scriptengine.h"
#include "scriptengine_services.h"
#include <Coral/StringHelper.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#define CH_SCRIPT_FUNC extern "C"

namespace Chained
{

// Cross-platform string conversion wrappers using Coral::StringHelper.
// On Windows: UCChar = wchar_t (UTF-16), on Linux/macOS: UCChar = char (UTF-8).
inline std::string ToUtf8(const Coral::UCChar* str)
{
    if (!str) return {};
    return Coral::StringHelper::ConvertWideToUtf8(str);
}

inline Coral::UCString ToWide(const std::string& str)
{
    return Coral::StringHelper::ConvertUtf8ToWide(str);
}

// Backward-compat aliases so existing .cpp files compile without changes.
static inline std::string ch_u16_to_string(const Coral::UCChar* ptr) { return ToUtf8(ptr); }
static inline std::u16string ch_utf8_to_u16(const std::string& str)
{
    // This function is kept only for rare call sites that genuinely need std::u16string.
    // Prefer ToWide() for new code.
    auto wide = ToWide(str);
    return std::u16string(wide.begin(), wide.end());
}

inline void ResolveComponentName(std::string& name)
{
    if (name == "MeshComponent") name = "ModelComponent";
    if (name == "PhysicsComponent") name = "ColliderComponent";
    // The registry derives SerializationKey from the human-readable display name
    // (RegisterReflective("Rigid Body") -> "Rigid BodyComponent"), but C# sends the
    // CLR type name typeof(RigidBodyComponent).Name == "RigidBodyComponent" (no space).
    // Without this alias the registry lookup never matches, so GetComponent<RigidBodyComponent>()
    // returns null and PlayerController.OnUpdate bails before applying WASD movement.
    if (name == "RigidBodyComponent") name = "Rigid BodyComponent";
}

Scene* GetActiveScene();

Entity GetEntity(uint64_t entityID);
} // namespace Chained

#endif // CH_SCRIPT_GLUE_INTERNAL_H
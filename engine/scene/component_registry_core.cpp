// Core component registrations (Transform, Tag, Camera, ID, Name, Hierarchy)
// Split into its own TU to reduce obj file size in MinGW Clang Debug builds.
#include "component_registry.h"
#include "components/camera_component.h"
#include "components/hierarchy_component.h"
#include "components/id_component.h"
#include "components/tag_component.h"
#include "components/transform_component.h"
#include "thirdparty/IconsFontAwesome6.h"

namespace Chained
{
void RegisterCoreComponents()
{
    ComponentRegistry::RegisterReflective<TransformComponent>("Transform", ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, "Core");
    ComponentRegistry::RegisterReflective<TagComponent>("Tag", ICON_FA_TAG, "Core");
    ComponentRegistry::RegisterReflective<CameraComponent>("Camera", ICON_FA_VIDEO, "Core");
    ComponentRegistry::RegisterReflective<IDComponent>("ID", nullptr, "Core");
    ComponentRegistry::RegisterReflective<NameComponent>("Name", nullptr, "Core");
    ComponentRegistry::RegisterReflective<HierarchyComponent>("Hierarchy", nullptr, "Core");
}
} // namespace Chained

#ifndef CH_TAG_COMPONENT_H
#define CH_TAG_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
struct TagComponent
{
    std::string Tag;
    static const char* GetStaticName() { return "TagComponent"; }

    
    struct UI
    {
        UIMeta Tag = {.Tooltip = "Custom tag for entity lookup and filtering"};
    };
};

CH_MARK_RFL(TagComponent);

} // namespace Chained

// TagComponent uses manual registration in RegisterEngineComponents()
// due to circular include dependency (entity.h -> tag_component.h -> component_registry.h)

#endif // CH_TAG_COMPONENT_H

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
        UIMeta Tag = {.Tooltip = "Користувацький тег для пошуку та фільтрації сутності"};
    };
};

CH_MARK_RFL(TagComponent);

} // namespace Chained

#endif // CH_TAG_COMPONENT_H

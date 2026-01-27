#ifndef CH_EXIT_SCRIPT_H
#define CH_EXIT_SCRIPT_H

#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"

namespace CHEngine
{
// Script for Exit button - closes the application when button is clicked
CH_SCRIPT(ExitScript){CH_UPDATE(dt){// Check if this entity has a ButtonWidget component
                                    if (GetEntity().HasComponent<ButtonWidget>()){
                                        auto &button = GetEntity().GetComponent<ButtonWidget>();

// If button was pressed this frame, close the application
} // namespace CHEngine
}
}
;
} // namespace CHEngine

#endif // CH_EXIT_SCRIPT_H
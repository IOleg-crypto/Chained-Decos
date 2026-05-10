#include "engine/core/service_locator.h"

namespace CHEngine
{
    std::unordered_map<std::type_index, void*> ServiceLocator::m_Services;
}

#include "service_locator.h"

namespace CHEngine {

    // The single master map for all services in the engine process.
    // By defining it in a .cpp file within engine.dll, we ensure there is only 
    // one instance of this map regardless of how many headers include service_locator.h
    std::unordered_map<std::type_index, std::shared_ptr<void>> ServiceLocator::m_Services;

} // namespace CHEngine

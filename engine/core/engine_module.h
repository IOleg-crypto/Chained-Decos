#ifndef CH_ENGINE_MODULE_H
#define CH_ENGINE_MODULE_H

#include "engine/foundation/base.h"
#include "engine/foundation/timestep.h"

namespace Chained {
class CH_API EngineModule {
    public:
    virtual ~EngineModule() = default;

    virtual void Initialize() = 0;
    virtual void Update(Timestep ts) = 0;
    virtual void Shutdown() = 0;

    bool IsEnabled() const {
        return m_Enabled;
    }
    void SetEnabled(bool enabled) {
        m_Enabled = enabled;
    }

    protected:
    bool m_Enabled = true;
};
} // namespace Chained

#endif
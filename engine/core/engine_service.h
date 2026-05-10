#ifndef CH_ENGINE_SERVICE_H
#define CH_ENGINE_SERVICE_H

#include "engine/core/timestep.h"

namespace CHEngine
{
    /**
     * @brief Base class for core engine subsystems using the Template Method pattern.
     * Guaranteed lifecycle flow: Start (OnInit -> OnPostInit) -> Tick (OnUpdate) -> Shutdown (OnShutdown)
     */
    class EngineService
    {
    public:
        virtual ~EngineService() = default;

        // Final template methods to control the execution skeleton
        void Start()
        {
            OnInit();
            OnPostInit();
        }

        void Tick(Timestep ts)
        {
            if (m_Enabled)
            {
                OnUpdate(ts);
            }
        }

        void Stop()
        {
            OnShutdown();
        }

        void SetEnabled(bool enabled) { m_Enabled = enabled; }
        bool IsEnabled() const { return m_Enabled; }

    protected:
        // Lifecycle hooks for derived services
        virtual void OnInit() {}
        virtual void OnPostInit() {}
        virtual void OnUpdate(Timestep ts) {}
        virtual void OnShutdown() = 0; // Shutdown remains mandatory but as a hook

    private:
        bool m_Enabled = true;
    };
}

#endif // CH_ENGINE_SERVICE_H

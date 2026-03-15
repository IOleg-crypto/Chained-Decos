#ifndef CH_SCRIPT_GLUE_H
#define CH_SCRIPT_GLUE_H

#include <Coral/Assembly.hpp>

namespace CHEngine {
    struct ManagedScriptInstance;

    class ScriptGlue {
    public:
        static void RegisterInternalCalls(Coral::ManagedAssembly& assembly);
        static void SetPendingScriptInstance(ManagedScriptInstance* instance);
    };
}

#endif // CH_SCRIPT_GLUE_H

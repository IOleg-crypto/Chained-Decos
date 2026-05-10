#ifndef CH_SCRIPT_GLUE_H
#define CH_SCRIPT_GLUE_H

#include <Coral/Assembly.hpp>

namespace CHEngine {
    class ScriptGlue {
    public:
        static void Initialize();
        static void RegisterInternalCalls(Coral::ManagedAssembly& assembly);
    };
}

#endif // CH_SCRIPT_GLUE_H

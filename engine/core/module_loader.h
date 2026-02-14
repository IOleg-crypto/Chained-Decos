#ifndef CH_MODULE_LOADER_H
#define CH_MODULE_LOADER_H

#include <string>

namespace CHEngine {

    class ScriptRegistry;

    class ModuleLoader {
    public:
        // ABI-safe: loads DLL, passes a C-callback for script registration
        static bool LoadGameModule(const std::string& dllPath, ScriptRegistry* registry);
        static void UnloadGameModule();
    };

}

#endif // CH_MODULE_LOADER_H

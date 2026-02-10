#ifndef CH_MODULE_LOADER_H
#define CH_MODULE_LOADER_H

#include "engine/scene/scene.h"
#include <string>

namespace CHEngine {

    class ModuleLoader {
    public:
        static bool LoadGameModule(const std::string& dllPath, Scene* scene);
        static void UnloadGameModule();
    };

}

#endif // CH_MODULE_LOADER_H

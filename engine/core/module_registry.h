#ifndef CH_MODULE_REGISTRY_H
#define CH_MODULE_REGISTRY_H

#include <vector>
#include <functional>

namespace CHEngine
{
    /**
     * @brief Centralized registry for the engine to discover and initialize 
     * game-specific modules without hardcoded dependencies.
     */
    class ModuleRegistry
    {
    public:
        using InitFunc = std::function<void()>;

        static void Register(InitFunc func)
        {
            GetInitFunctions().push_back(func);
        }

        static void InitializeAll()
        {
            for (auto& func : GetInitFunctions())
            {
                func();
            }
        }

    private:
        static std::vector<InitFunc>& GetInitFunctions()
        {
            static std::vector<InitFunc> s_Functions;
            return s_Functions;
        }
    };

    /**
     * @brief Macro to register a game initialization function.
     * Use this in any .cpp file to run code on engine startup.
     */
    #define CH_REGISTER_MODULE_INIT(func) \
        static bool s_ModuleInitialized_##func = []() { \
            ::CHEngine::ModuleRegistry::Register(func); \
            return true; \
        }()

} // namespace CHEngine

#endif // CH_MODULE_REGISTRY_H

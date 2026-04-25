#ifndef CH_SCRIPT_INTERNAL_CALL_REGISTRY_H
#define CH_SCRIPT_INTERNAL_CALL_REGISTRY_H

#include <string>
#include <vector>
#include <functional>

namespace CHEngine
{
    struct InternalCallMapping
    {
        std::string ClassName;
        std::string MethodName;
        void* FuncPtr;
    };

    class InternalCallRegistry
    {
    public:
        static void Add(const InternalCallMapping& mapping)
        {
            GetMappings().push_back(mapping);
        }

        static std::vector<InternalCallMapping>& GetMappings()
        {
            static std::vector<InternalCallMapping> s_Mappings;
            return s_Mappings;
        }
    };

    struct InternalCallRegister
    {
        InternalCallRegister(const std::string& className, const std::string& methodName, void* funcPtr)
        {
            InternalCallRegistry::Add({ className, methodName, funcPtr });
        }
    };
}

#define CH_ADD_INTERNAL_CALL(className, methodName, funcPtr) \
    static CHEngine::InternalCallRegister s_##methodName##_Registrar("CHEngine." #className, #methodName, (void*)funcPtr)

#endif // CH_SCRIPT_INTERNAL_CALL_REGISTRY_H

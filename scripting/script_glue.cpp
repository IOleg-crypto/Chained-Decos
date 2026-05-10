#include "script_glue_internal.h"
#include "script_internal_call_registry.h"

namespace CHEngine {

    void ScriptGlue::Initialize()
    {
        RegisterGlueSystem();
        RegisterGlueInput();
        RegisterGlueNetwork();
        RegisterGlueScene();
        RegisterGlueEntity();
        RegisterGlueCamera();
        RegisterGlueUI();
        RegisterGlueAudio();
    }

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        CH_CORE_INFO("ScriptEngine: Registering internal calls for assembly '{}'...", (std::string)assembly.GetName());

        uint32_t registeredCount = 0;
        for (const auto& mapping : InternalCallRegistry::GetMappings())
        {
            // Only register the internal call if the type belongs to THIS assembly.
            // This prevents "Type not found" errors when we have multiple assemblies (Core + App).
            if (assembly.GetLocalType(mapping.ClassName))
            {
                assembly.AddInternalCall(mapping.ClassName, mapping.MethodName, mapping.FuncPtr);
                registeredCount++;
            }
        }

        if (registeredCount > 0)
        {
            assembly.UploadInternalCalls();
            CH_CORE_INFO("ScriptEngine: Uploaded {} internal calls for assembly '{}'.", registeredCount, (std::string)assembly.GetName());
        }
    }

} // namespace CHEngine

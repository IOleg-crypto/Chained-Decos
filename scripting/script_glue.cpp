#include "script_glue_internal.h"
#include "script_internal_call_registry.h"

namespace CHEngine {

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        for (const auto& mapping : InternalCallRegistry::GetMappings())
        {
            assembly.AddInternalCall(mapping.ClassName, mapping.MethodName, mapping.FuncPtr);
        }

        assembly.UploadInternalCalls();
    }

} // namespace CHEngine

#include "script_glue_internal.h"
#include "script_internal_call_registry.h"

#include "script_glue.h"

namespace Chained {

    void ScriptGlue::Initialize()
    {
    }



    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        auto& mappings = InternalCallRegistry::GetMappings();
        for (const auto& mapping : mappings)
        {
            assembly.AddInternalCall(mapping.ClassName, mapping.MethodName, mapping.FuncPtr);
        }

        if (!mappings.empty())
        {
            assembly.UploadInternalCalls();
            CH_CORE_INFO("ScriptEngine: Uploaded {} internal calls for assembly '{}'.", mappings.size(), (std::string)assembly.GetName());
        }
    }

} // namespace Chained

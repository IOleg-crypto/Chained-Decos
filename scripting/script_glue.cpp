
#include "script_glue.h"

namespace Chained {

    extern void RegisterGlueUI(Coral::ManagedAssembly& assembly);
    extern void RegisterGlueSystem(Coral::ManagedAssembly& assembly);
    extern void RegisterGlueScene(Coral::ManagedAssembly& assembly);
    extern void RegisterGlueInput(Coral::ManagedAssembly& assembly);
    extern void RegisterGlueEntity(Coral::ManagedAssembly& assembly);
    extern void RegisterGlueCamera(Coral::ManagedAssembly& assembly);
    extern void RegisterGlueAudio(Coral::ManagedAssembly& assembly);

    void ScriptGlue::Initialize()
    {
    }

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        RegisterGlueUI(assembly);
        RegisterGlueSystem(assembly);
        RegisterGlueScene(assembly);
        RegisterGlueInput(assembly);
        RegisterGlueEntity(assembly);
        RegisterGlueCamera(assembly);
        RegisterGlueAudio(assembly);

        assembly.UploadInternalCalls();
        CH_CORE_INFO("ScriptEngine: Uploaded internal calls for assembly '{}'.", (std::string)assembly.GetName());
    }

} // namespace Chained

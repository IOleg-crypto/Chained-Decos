#include "script_glue_internal.h"

namespace CHEngine {

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        RegisterSystemInternalCalls(assembly);
        RegisterInputInternalCalls(assembly);
        RegisterSceneInternalCalls(assembly);
        RegisterEntityInternalCalls(assembly);
        RegisterCameraInternalCalls(assembly);
        RegisterUIInternalCalls(assembly);
        RegisterGameplayInternalCalls(assembly);

        // Finalize registration
        assembly.UploadInternalCalls();
    }

} // namespace CHEngine

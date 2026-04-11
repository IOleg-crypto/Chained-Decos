#include "script_glue_internal.h"

namespace CHEngine {

    void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly) {
        RegisterSystemGlue(assembly);
        RegisterInputGlue(assembly);
        RegisterSceneGlue(assembly);
        RegisterEntityGlue(assembly);
        RegisterCameraGlue(assembly);
        RegisterUIGlue(assembly);
        RegisterGameplayGlue(assembly);

        assembly.UploadInternalCalls();
    }

} // namespace CHEngine

// script_glue.h
// Registers C++ functions as Coral internal calls.
// Each function gets its own AddInternalCall — Coral fills the C# delegate* fields.
#ifndef CH_SCRIPT_GLUE_H
#define CH_SCRIPT_GLUE_H

namespace Coral
{
class ManagedAssembly;
}

namespace Chained
{
class ScriptGlue
{
public:
    // Registers all C++ ↔ C# interop functions with Coral.
    static void RegisterInternalCalls(Coral::ManagedAssembly& assembly);
};
} // namespace Chained

#endif // CH_SCRIPT_GLUE_H

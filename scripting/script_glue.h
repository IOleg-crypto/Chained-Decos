#ifndef CH_SCRIPT_GLUE_H
#define CH_SCRIPT_GLUE_H

namespace Coral
{
class ManagedAssembly;
}

namespace Chained
{
struct ChainedNativePointers; 
struct ChainedManagedPointers;

class ScriptGlue
{
public:
    static void Initialize();
    static void RegisterInternalCalls(Coral::ManagedAssembly& assembly);
    static void Native_BypassInit(void* outNative, void* inManaged);
    static void FillNativePointers(ChainedNativePointers* ptrs);
};
} // namespace Chained

#endif // CH_SCRIPT_GLUE_H
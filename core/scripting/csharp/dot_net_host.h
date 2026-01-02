#ifndef CD_CORE_SCRIPTING_CSHARP_DOT_NET_HOST_H
#define CD_CORE_SCRIPTING_CSHARP_DOT_NET_HOST_H

#include <filesystem>
#include <string>
#include <vector>


namespace CHEngine
{
class DotNetHost
{
public:
    static bool Init();
    static void Shutdown();

    // Load a managed assembly and get a function pointer
    static void *GetManagedFunction(const std::wstring &assemblyPath, const std::wstring &typeName,
                                    const std::wstring &methodName);

private:
    static bool LoadHostfxr();
    static void *GetExport(void *h, const char *name);

private:
    inline static void *s_HostfxrLib = nullptr;
    inline static bool s_Initialized = false;
};
} // namespace CHEngine

#endif // CD_CORE_SCRIPTING_CSHARP_DOT_NET_HOST_H

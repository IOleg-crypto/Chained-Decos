#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <Windows.h>

#include "DotNetHost.h"
#include "core/Log.h"

#include <algorithm>
#include <filesystem>
#include <vector>

// --- Manual Hostfxr Definitions ---
#ifdef _WIN32
typedef wchar_t char_t;
#else
typedef char char_t;
#endif

typedef void *hostfxr_handle;

enum hostfxr_delegate_type
{
    hdt_com_activation = 0,
    hdt_load_in_memory_assembly = 1,
    hdt_load_assembly_and_get_function_pointer = 2,
    hdt_get_function_pointer = 3,
    hdt_com_component_activator = 4,
    hdt_com_best_effort_at_loading_managed_assembly = 5,
    hdt_type_marshaler_base = 6
};

#define UNMANAGEDCALLERSONLY_METHOD ((const char_t *)-1)

typedef int (*hostfxr_initialize_for_runtime_config_fn)(const char_t *runtime_config_path,
                                                        const void *parameters,
                                                        hostfxr_handle *host_context_handle);

typedef int (*hostfxr_get_runtime_delegate_fn)(hostfxr_handle host_context_handle,
                                               enum hostfxr_delegate_type type, void **delegate);

typedef int (*hostfxr_close_fn)(hostfxr_handle host_context_handle);

#ifdef _WIN32
#define CORECLR_DELEGATE_CALLTYPE __stdcall
#else
#define CORECLR_DELEGATE_CALLTYPE
#endif

namespace CHEngine
{
using load_assembly_and_get_function_pointer_fn = int(CORECLR_DELEGATE_CALLTYPE *)(
    const char_t *assembly_path, const char_t *type_name, const char_t *method_name,
    const char_t *delegate_type_name, void *reserved, void **delegate);

static hostfxr_initialize_for_runtime_config_fn init_fptr;
static hostfxr_get_runtime_delegate_fn get_delegate_fptr;
static hostfxr_close_fn close_fptr;

// Manual resolver for hostfxr
static std::wstring FindHostfxr()
{
    std::wstring dotnetRoot = L"C:\\Program Files\\dotnet";
    std::filesystem::path fxrPath = std::filesystem::path(dotnetRoot) / "host" / "fxr";

    if (!std::filesystem::exists(fxrPath))
        return L"";

    std::vector<std::filesystem::path> versions;
    try
    {
        for (auto &p : std::filesystem::directory_iterator(fxrPath))
        {
            if (p.is_directory())
                versions.push_back(p.path());
        }
    }
    catch (...)
    {
        return L"";
    }

    if (versions.empty())
        return L"";

    // Sort to get latest version
    std::sort(versions.begin(), versions.end());
    return (versions.back() / L"hostfxr.dll").wstring();
}

bool DotNetHost::Init()
{
    if (s_Initialized)
        return true;

    CD_CORE_INFO("Initializing .NET Host...");

    std::wstring hostfxrPath = FindHostfxr();
    if (hostfxrPath.empty())
    {
        CD_CORE_ERROR("Failed to find hostfxr.dll!");
        return false;
    }

    CD_CORE_INFO("Found hostfxr: %ls", hostfxrPath.c_str());

    s_HostfxrLib = (void *)LoadLibraryW(hostfxrPath.c_str());
    if (!s_HostfxrLib)
    {
        CD_CORE_ERROR("Failed to load hostfxr library!");
        return false;
    }

    init_fptr = (hostfxr_initialize_for_runtime_config_fn)GetExport(
        s_HostfxrLib, "hostfxr_initialize_for_runtime_config");
    get_delegate_fptr =
        (hostfxr_get_runtime_delegate_fn)GetExport(s_HostfxrLib, "hostfxr_get_runtime_delegate");
    close_fptr = (hostfxr_close_fn)GetExport(s_HostfxrLib, "hostfxr_close");

    if (!init_fptr || !get_delegate_fptr || !close_fptr)
    {
        CD_CORE_ERROR("Failed to get hostfxr exports!");
        return false;
    }

    s_Initialized = true;
    CD_CORE_INFO(".NET Host initialized successfully.");
    return true;
}

void DotNetHost::Shutdown()
{
    if (s_HostfxrLib)
    {
        FreeLibrary((HMODULE)s_HostfxrLib);
        s_HostfxrLib = nullptr;
    }
    s_Initialized = false;
}

void *DotNetHost::GetExport(void *h, const char *name)
{
    if (!h)
        return nullptr;
    return (void *)GetProcAddress((HMODULE)h, name);
}

void *DotNetHost::GetManagedFunction(const std::wstring &assemblyPath, const std::wstring &typeName,
                                     const std::wstring &methodName)
{
    if (!s_Initialized && !Init())
        return nullptr;

    CD_CORE_INFO("Loading managed assembly: %ls", assemblyPath.c_str());

    hostfxr_handle ctx = nullptr;
    std::wstring configPath =
        std::filesystem::path(assemblyPath).replace_extension(L".runtimeconfig.json").wstring();

    int rc = init_fptr(configPath.c_str(), nullptr, &ctx);
    if (rc != 0 || ctx == nullptr)
    {
        CD_CORE_ERROR("Failed to initialize for runtime config: %ls (rc: %08x)", configPath.c_str(),
                      rc);
        if (ctx)
            close_fptr(ctx);
        return nullptr;
    }

    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
    rc = get_delegate_fptr(ctx, hdt_load_assembly_and_get_function_pointer,
                           (void **)&load_assembly_and_get_function_pointer);
    if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
    {
        CD_CORE_ERROR("Failed to get load assembly delegate! (rc: %08x)", rc);
        close_fptr(ctx);
        return nullptr;
    }

    close_fptr(ctx);

    void *fptr = nullptr;
    rc = load_assembly_and_get_function_pointer(assemblyPath.c_str(), typeName.c_str(),
                                                methodName.c_str(), UNMANAGEDCALLERSONLY_METHOD,
                                                nullptr, &fptr);

    if (rc != 0 || fptr == nullptr)
    {
        CD_CORE_ERROR("Failed to get managed function pointer! %ls:%ls (rc: %08x)",
                      typeName.c_str(), methodName.c_str(), rc);
        return nullptr;
    }

    return fptr;
}
} // namespace CHEngine

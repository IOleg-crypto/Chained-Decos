#include "platform.h"
#include <GLFW/glfw3.h>
#include <nfd.h>

#if defined(CH_PLATFORM_WINDOWS)
    #include <windows.h>
#elif defined(CH_PLATFORM_LINUX)
    #include <unistd.h>
    #include <pthread.h>
#endif

namespace CHEngine
{
    std::filesystem::path Platform::GetExecutableDirectory()
    {
#if defined(CH_PLATFORM_WINDOWS)
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::filesystem::path(path).parent_path();
#elif defined(CH_PLATFORM_LINUX)
        char path[1024];
        ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
        return std::filesystem::path(std::string(path, (count > 0) ? count : 0)).parent_path();
#else
        return std::filesystem::current_path();
#endif
    }

    float Platform::GetTime()
    {
        return (float)glfwGetTime();
    }

    void Platform::Sleep(uint32_t milliseconds)
    {
#if defined(CH_PLATFORM_WINDOWS)
        ::Sleep(milliseconds);
#elif defined(CH_PLATFORM_LINUX)
        usleep(milliseconds * 1000);
#endif
    }

    void Platform::SetThreadName(const std::string& name)
    {
#if defined(CH_PLATFORM_WINDOWS)
        std::wstring wname(name.begin(), name.end());
        SetThreadDescription(GetCurrentThread(), wname.c_str());
#elif defined(CH_PLATFORM_LINUX)
        pthread_setname_np(pthread_self(), name.c_str());
#endif
    }

    std::optional<std::filesystem::path> Platform::OpenFile(const std::vector<FileDialogFilter>& filters)
    {
        nfdu8char_t* outPath = NULL;
        std::vector<nfdu8filteritem_t> nfdFilters;
        for (const auto& filter : filters)
        {
            nfdFilters.push_back({filter.Name.c_str(), filter.Spec.c_str()});
        }

        nfdresult_t result = NFD_OpenDialogU8(&outPath, nfdFilters.data(), (nfdfiltersize_t)nfdFilters.size(), NULL);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path = outPath;
            NFD_FreePathU8(outPath);
            return path;
        }
        return std::nullopt;
    }

    std::optional<std::filesystem::path> Platform::SaveFile(const std::vector<FileDialogFilter>& filters)
    {
        nfdu8char_t* outPath = NULL;
        std::vector<nfdu8filteritem_t> nfdFilters;
        for (const auto& filter : filters)
        {
            nfdFilters.push_back({filter.Name.c_str(), filter.Spec.c_str()});
        }

        nfdresult_t result = NFD_SaveDialogU8(&outPath, nfdFilters.data(), (nfdfiltersize_t)nfdFilters.size(), NULL, NULL);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path = outPath;
            NFD_FreePathU8(outPath);
            return path;
        }
        return std::nullopt;
    }

    std::optional<std::filesystem::path> Platform::PickFolder()
    {
        nfdu8char_t* outPath = NULL;
        nfdresult_t result = NFD_PickFolderU8(&outPath, NULL);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path = outPath;
            NFD_FreePathU8(outPath);
            return path;
        }
        return std::nullopt;
    }
}

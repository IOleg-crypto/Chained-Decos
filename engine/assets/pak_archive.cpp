#include "engine/assets/pak_archive.h"

#include "gpak.h"
#include "gpak_data.h"

#include <algorithm>

namespace Chained
{

PakArchive::PakArchive(gpak_t* handle, std::filesystem::path path)
    : m_Handle(handle), m_Path(std::move(path))
{
}

PakArchive::~PakArchive()
{
    if (m_Handle)
    {
        gpak_close(m_Handle);
        m_Handle = nullptr;
    }
}

PakArchive::PakArchive(PakArchive&& other) noexcept
    : m_Handle(other.m_Handle), m_Path(std::move(other.m_Path))
{
    other.m_Handle = nullptr;
}

PakArchive& PakArchive::operator=(PakArchive&& other) noexcept
{
    if (this != &other)
    {
        if (m_Handle)
        {
            gpak_close(m_Handle);
        }
        m_Handle = other.m_Handle;
        m_Path = std::move(other.m_Path);
        other.m_Handle = nullptr;
    }
    return *this;
}

std::unique_ptr<PakArchive> PakArchive::OpenReadOnly(const std::filesystem::path& path)
{
    gpak_t* handle = gpak_open(path.string().c_str(), GPAK_MODE_READ_ONLY);
    if (!handle)
    {
        return nullptr;
    }
    // std::unique_ptr can't call a private constructor via make_unique, so construct directly.
    return std::unique_ptr<PakArchive>(new PakArchive(handle, path));
}

std::unique_ptr<PakArchive> PakArchive::Create(const std::filesystem::path& path)
{
    gpak_t* handle = gpak_open(path.string().c_str(), GPAK_MODE_CREATE);
    if (!handle)
    {
        return nullptr;
    }
    return std::unique_ptr<PakArchive>(new PakArchive(handle, path));
}

void PakArchive::SetCompression(int algorithm, int level)
{
    if (!m_Handle)
    {
        return;
    }
    gpak_set_compression_algorithm(m_Handle, algorithm);
    gpak_set_compression_level(m_Handle, level);
}

bool PakArchive::AddFile(const std::filesystem::path& externalPath, std::string_view internalPath)
{
    if (!m_Handle)
    {
        return false;
    }
    std::string internal(internalPath);
    return gpak_add_file(m_Handle, externalPath.string().c_str(), internal.c_str()) == 0;
}

bool PakArchive::AddDirectory(std::string_view internalPath)
{
    if (!m_Handle)
    {
        return false;
    }
    std::string internal(internalPath);
    return gpak_add_directory(m_Handle, internal.c_str()) == 0;
}

bool PakArchive::Exists(std::string_view internalPath) const
{
    if (!m_Handle)
    {
        return false;
    }
    std::string internal(internalPath);
    return gpak_find_file(m_Handle, internal.c_str()) != nullptr;
}

std::vector<uint8_t> PakArchive::ReadFile(std::string_view internalPath) const
{
    std::vector<uint8_t> result;
    if (!m_Handle)
    {
        return result;
    }

    std::string internal(internalPath);
    gpak_file_t* file = gpak_fopen(m_Handle, internal.c_str());
    if (!file)
    {
        return result;
    }

    constexpr size_t kChunkSize = 64 * 1024;
    uint8_t chunk[kChunkSize];
    size_t read = 0;
    while ((read = gpak_fread(chunk, 1, kChunkSize, file)) > 0)
    {
        result.insert(result.end(), chunk, chunk + read);
    }

    gpak_fclose(file);
    return result;
}

void PakArchiveManager::Mount(std::string_view mountName, std::unique_ptr<PakArchive> archive)
{
    if (!archive)
    {
        return;
    }
    m_Mounted.emplace_back(std::string(mountName), std::move(archive));
}

void PakArchiveManager::Unmount(std::string_view mountName)
{
    m_Mounted.erase(std::remove_if(m_Mounted.begin(), m_Mounted.end(),
                         [mountName](const auto& entry) { return entry.first == mountName; }),
        m_Mounted.end());
}

bool PakArchiveManager::Exists(std::string_view internalPath) const
{
    // Last-mounted-wins overlay: search from the back so later mounts (dlc/mods) shadow earlier ones.
    for (auto it = m_Mounted.rbegin(); it != m_Mounted.rend(); ++it)
    {
        if (it->second->Exists(internalPath))
        {
            return true;
        }
    }
    return false;
}

std::vector<uint8_t> PakArchiveManager::ReadFile(std::string_view internalPath) const
{
    for (auto it = m_Mounted.rbegin(); it != m_Mounted.rend(); ++it)
    {
        if (it->second->Exists(internalPath))
        {
            return it->second->ReadFile(internalPath);
        }
    }
    return {};
}

} // namespace Chained

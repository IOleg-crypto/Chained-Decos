#ifndef CH_PAK_ARCHIVE_H
#define CH_PAK_ARCHIVE_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

struct gpak;
struct gpak_file;
typedef struct gpak gpak_t;
typedef struct gpak_file gpak_file_t;

namespace Chained
{

// RAII wrapper around a single opened .gpak archive (libgpak's gpak_t*).
// One instance == one mounted archive. To overlay several archives (base + dlc + mods),
// use PakArchiveManager rather than trying to reuse a single PakArchive.
class PakArchive
{
public:
    ~PakArchive();

    PakArchive(const PakArchive&) = delete;
    PakArchive& operator=(const PakArchive&) = delete;
    PakArchive(PakArchive&& other) noexcept;
    PakArchive& operator=(PakArchive&& other) noexcept;

    // Opens an existing archive for reading. Returns nullptr on failure.
    [[nodiscard]] static std::unique_ptr<PakArchive> OpenReadOnly(const std::filesystem::path& path);

    // Creates a new archive for writing/packing. Returns nullptr on failure.
    [[nodiscard]] static std::unique_ptr<PakArchive> Create(const std::filesystem::path& path);

    // Packing helpers — only valid on archives opened via Create().
    void SetCompression(int algorithm, int level);
    bool AddFile(const std::filesystem::path& externalPath, std::string_view internalPath);
    bool AddDirectory(std::string_view internalPath);

    [[nodiscard]] bool Exists(std::string_view internalPath) const;

    // Reads a whole file into memory. Returns an empty vector on failure (check Exists() first
    // if a zero-byte file vs. a missing file needs to be distinguished).
    [[nodiscard]] std::vector<uint8_t> ReadFile(std::string_view internalPath) const;

    [[nodiscard]] const std::filesystem::path& GetPath() const { return m_Path; }

private:
    PakArchive(gpak_t* handle, std::filesystem::path path);

    gpak_t* m_Handle = nullptr;
    std::filesystem::path m_Path;
};

// Owns a stack of mounted PakArchive instances and resolves reads through them, last-mounted-wins
// (mirrors typical overlay-VFS behaviour: base.pak mounted first, dlc.pak / mods.pak layered on top).
class PakArchiveManager
{
public:
    // mountName is informational only (logging/inspection) — lookups are by internal path.
    void Mount(std::string_view mountName, std::unique_ptr<PakArchive> archive);
    void Unmount(std::string_view mountName);

    [[nodiscard]] bool Exists(std::string_view internalPath) const;
    [[nodiscard]] std::vector<uint8_t> ReadFile(std::string_view internalPath) const;

private:
    std::vector<std::pair<std::string, std::unique_ptr<PakArchive>>> m_Mounted;
};

} // namespace Chained

#endif // CH_PAK_ARCHIVE_H

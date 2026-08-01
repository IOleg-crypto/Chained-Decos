#ifndef CH_PACK_IO_H
#define CH_PACK_IO_H

#include <assimp/IOSystem.hpp>
#include <assimp/IOStream.hpp>
#include <vector>
#include <string>

namespace Chained
{

class PackIOStream : public Assimp::IOStream
{
    friend class PackIOSystem;

    std::vector<char> m_Data;
    size_t m_Pos = 0;

    PackIOStream(std::vector<char> data)
        : m_Data(std::move(data))
    {
    }

public:
    ~PackIOStream() override = default;

    size_t Read(void* pBuffer, size_t pSize, size_t pCount) override;
    size_t Write(const void*, size_t, size_t) override;
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override;
    size_t Tell() const override;
    size_t FileSize() const override;
    void Flush() override;
};

class PackIOSystem : public Assimp::IOSystem
{
    std::string m_BaseDir;
    bool m_Packed;

public:
    PackIOSystem(const std::string& baseDir, bool packed);
    ~PackIOSystem() override = default;

    bool ChangeDirectory(const std::string&) override;
    const std::string& CurrentDirectory() const override;
    bool Exists(const char* pFile) const override;
    char getOsSeparator() const override;
    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override;
    void Close(Assimp::IOStream* pFile) override;
};

} // namespace Chained

#endif // CH_PACK_IO_H

#ifndef CH_ASSET_H
#define CH_ASSET_H

#include "engine/core/base.h"
#include "engine/core/uuid.h"
#include "string"

namespace CHEngine
{

enum class AssetType : uint16_t
{
    None = 0,
    Model,
    Texture,
    Audio,
    Shader,
    Environment,
    Material,
    Font
};

enum class AssetState : uint8_t
{
    None = 0,
    Loading,
    Ready,
    Failed
};

class Asset
{
public:
    virtual ~Asset() = default;

    virtual AssetType GetType() const = 0;

    AssetState GetState() const
    {
        return m_State;
    }
    void SetState(AssetState state)
    {
        m_State = state;
    }

    bool IsReady() const
    {
        return m_State == AssetState::Ready;
    }

    const std::string &GetPath() const
    {
        return m_Path;
    }
    void SetPath(const std::string &path)
    {
        m_Path = path;
    }

    UUID GetID() const
    {
        return m_ID;
    }

protected:
    std::string m_Path;
    UUID m_ID;
    AssetState m_State = AssetState::None;
};

} // namespace CHEngine

#endif // CH_ASSET_H

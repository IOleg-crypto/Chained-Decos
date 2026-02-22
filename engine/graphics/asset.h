#ifndef CH_ASSET_H
#define CH_ASSET_H

#include "engine/core/base.h"
#include "engine/core/uuid.h"
#include <cstdio>
#include <string>
#include <atomic>

namespace CHEngine
{
using AssetHandle = UUID;

enum class AssetType : uint16_t
{
    None = 0,
    Model,
    Texture,
    Audio,
    Shader,
    Environment,
    Material,
    Font,
    AnimationGraph
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

    AssetType GetType() const
    {
        return m_Type;
    }

    Asset(AssetType type = AssetType::None)
        : m_Type(type),
          m_ID()
    {
    }
    Asset(AssetType type, UUID id)
        : m_Type(type),
          m_ID(id)
    {
    }

    AssetState GetState() const
    {
        return m_State.load(std::memory_order_relaxed);
    }
    void SetState(AssetState state)
    {
        AssetState oldState = m_State.exchange(state, std::memory_order_release);
        if (state != oldState)
        {
            printf("[ASSET] '%s' state change: %d -> %d\n", m_Path.c_str(), (int)oldState, (int)state);
            fflush(stdout);
            
            if (state == AssetState::Failed)
            {
                CH_CORE_WARN("Asset: FAILED for '{}' (Type: {}, ID: {})", m_Path, (int)m_Type, (uint64_t)m_ID);
            }
        }
    }

    bool IsReady() const
    {
        return m_State == AssetState::Ready;
    }

    const std::string& GetPath() const
    {
        return m_Path;
    }
    void SetPath(const std::string& path)
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
    AssetType m_Type = AssetType::None;
    std::atomic<AssetState> m_State = AssetState::None;
};

} // namespace CHEngine

#endif // CH_ASSET_H

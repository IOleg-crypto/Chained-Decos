#ifndef CH_ASSET_H
#define CH_ASSET_H

#include "engine/common/uuid.h"
#include "engine/core/log.h"
#include <cstdio>
#include <string>
#include <atomic>

namespace Chained
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
    Scene,
    Script,
    AnimationGraph // Not existing yet, reserved for future use
};
// Asset loading state.
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

    // Returns the asset type discriminator.
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

    // Returns the current asset loading state using relaxed atomic reads.
    AssetState GetState() const
    {
        return m_State.load(std::memory_order_relaxed);
    }
    // Updates the loading state and logs state transitions for diagnostics.
    void SetState(AssetState state)
    {
        AssetState oldState = m_State.exchange(state, std::memory_order_release);
        if (state != oldState)
        {
            printf("[ASSET] '%s' state change: %d -> %d\n", m_Path.c_str(), (int)oldState, (int)state);
            fflush(stdout);
            
            if (state == AssetState::Failed)
            {
                if (!m_Error.empty())
                {
                    CH_CORE_WARN("Asset: FAILED for '{}' (Type: {}, ID: {}) - {}", m_Path, (int)m_Type,
                                 (uint64_t)m_ID, m_Error);
                }
                else
                {
                    CH_CORE_WARN("Asset: FAILED for '{}' (Type: {}, ID: {})", m_Path, (int)m_Type, (uint64_t)m_ID);
                }
            }
        }
    }

    // Returns the last loader or finalization error, if any.
    const std::string& GetError() const
    {
        return m_Error;
    }

    // Stores a descriptive error message without changing the state.
    void SetError(const std::string& error)
    {
        m_Error = error;
    }

    // Clears the stored error message.
    void ClearError()
    {
        m_Error.clear();
    }

    // Marks the asset as failed and records the failure message.
    void Fail(const std::string& error)
    {
        m_Error = error;
        SetState(AssetState::Failed);
    }

    // Returns true when the asset reached the ready state.
    bool IsReady() const
    {
        return m_State == AssetState::Ready;
    }

    // Returns the resolved asset path used by the loader.
    const std::string& GetPath() const
    {
        return m_Path;
    }
    // Updates the resolved asset path used by the loader.
    void SetPath(const std::string& path)
    {
        m_Path = path;
    }

    // Returns the stable UUID handle associated with this asset.
    UUID GetID() const
    {
        return m_ID;
    }

    // Assigns the stable UUID handle, typically loaded from a .meta sidecar file.
    void SetID(UUID id)
    {
        m_ID = id;
    }

    // Called on the main thread after loading completes; useful for GPU uploads.
    virtual void OnLoaded() {}
protected:
    std::string m_Path;
    std::string m_Error;
    UUID m_ID;
    AssetType m_Type = AssetType::None;
    std::atomic<AssetState> m_State = AssetState::None;
};

} // namespace Chained

#endif // CH_ASSET_H
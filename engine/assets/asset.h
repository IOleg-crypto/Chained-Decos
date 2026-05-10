#ifndef CH_ASSET_H
#define CH_ASSET_H

#include "engine/core/base.h"
#include "engine/core/uuid.h"

#include <atomic>
#include <chrono>
#include <string>

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

    Asset(AssetType type = AssetType::None)
        : m_Type(type),
          m_ID()
    {
        m_StartTime = std::chrono::steady_clock::now();
    }
    Asset(AssetType type, UUID id)
        : m_Type(type),
          m_ID(id)
    {
        m_StartTime = std::chrono::steady_clock::now();
    }

    // Returns the asset type discriminator.
    AssetType GetType() const { return m_Type; }

    // Returns the stable UUID handle associated with this asset.
    UUID GetID() const { return m_ID; }

    // Returns the path hint for this asset (source data).
    const std::string& GetPath() const { return m_Path; }
    void SetPath(const std::string& path) { m_Path = path; }

    // Override the auto-generated Handle for deterministic setups
    void OverrideID(UUID newId) { m_ID = newId; }

    // Returns the current asset loading state using relaxed atomic reads.
    AssetState GetState() const { return m_State.load(std::memory_order_relaxed); }
    
    void SetState(AssetState state) { m_State.store(state, std::memory_order_release); }

    // Returns true when the asset reached the ready state.
    bool IsReady() const { return m_State == AssetState::Ready; }

    // Called on the main thread after loading completes; useful for GPU uploads.
    virtual void OnLoaded() {}

    std::chrono::steady_clock::time_point GetStartTime() const { return m_StartTime; }

protected:
    UUID m_ID;
    AssetType m_Type = AssetType::None;
    std::string m_Path;
    std::atomic<AssetState> m_State = AssetState::None;
    std::chrono::steady_clock::time_point m_StartTime;
};

} // namespace CHEngine

#endif // CH_ASSET_H

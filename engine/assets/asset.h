#ifndef CH_ASSET_H
#define CH_ASSET_H

#include "engine/foundation/base.h"
#include "engine/foundation/uuid.h"

#include <atomic>
#include <chrono>
#include <string>

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
        AnimationGraph
    };

    enum class AssetState : uint8_t
    {
        None = 0,
        Loading,
        Ready,
        Failed
    };

    class CH_API Asset
    {
    public:
        virtual ~Asset() = default;

        Asset(const Asset&) = delete;
        Asset& operator=(const Asset&) = delete;
        Asset(Asset&&) noexcept = delete;
        Asset& operator=(Asset&&) noexcept = delete;

        AssetType GetType() const { return m_Type; }
        UUID GetID() const { return m_ID; }
        void OverrideID(UUID newId) { m_ID = newId; }

        const std::string& GetPath() const { return m_Path; }
        void SetPath(const std::string& path) { m_Path = path; }

        AssetState GetState() const { return m_State.load(std::memory_order_acquire); }
        void SetState(AssetState state) { m_State.store(state, std::memory_order_release); }

        bool IsReady() const { return GetState() == AssetState::Ready; }

        virtual size_t GetMemoryUsage() const = 0;

        std::chrono::steady_clock::time_point GetStartTime() const { return m_StartTime; }

    protected:
        explicit Asset(AssetType type = AssetType::None)
            : m_Type(type), m_StartTime(std::chrono::steady_clock::now()) {}

        Asset(AssetType type, UUID id);

    protected:
        UUID m_ID;
        AssetType m_Type = AssetType::None;
        std::string m_Path;
        std::atomic<AssetState> m_State{ AssetState::None };
        std::chrono::steady_clock::time_point m_StartTime;
    };

    inline Asset::Asset(AssetType type, UUID id): m_ID(id), m_Type(type), m_StartTime(std::chrono::steady_clock::now()) {}
} // namespace Chained

#endif // CH_ASSET_H
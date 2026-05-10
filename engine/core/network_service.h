#ifndef CH_NETWORK_SERVICE_H
#define CH_NETWORK_SERVICE_H

#include "engine/core/engine_service.h"
#include <cstdint>
#include <vector>

namespace CHEngine
{
    /**
     * @brief Core engine service for multiplayer networking using SteamNetworkingSockets.
     * Uses opaque handles (uint32_t) to avoid header pollution in the public API.
     */
    class NetworkService : public ::CHEngine::EngineService
    {
    public:
        NetworkService();
        virtual ~NetworkService() override;

        // Startup/Shutdown
        bool Host(uint16_t port);
        bool Connect(const char* address);
        void Disconnect();

        // Message Processing
        void Poll();
        
        // Data Transfer
        bool SendData(const void* data, uint32_t size, bool reliable = true);
        bool Broadcast(const void* data, uint32_t size, bool reliable = true);
        bool HasMessages() const { return !m_MessageQueue.empty(); }
        std::vector<uint8_t> GetNextMessage();

        bool IsServer() const { return m_IsServer; }
        bool IsClient() const { return m_IsClient; }
        bool IsActive() const { return m_IsServer || m_IsClient; }

    protected:
        virtual void OnInit() override;
        virtual void OnUpdate(Timestep ts) override;
        virtual void OnShutdown() override;

    private:
        void InitializeSteamNetworking();
        
        // Internal callback handler
        static void OnSteamNetConnectionStatusChangedInternal(void* pInfo);
        void HandleConnectionStatusChanged(void* pInfo);

    private:
        uint32_t m_ListenSocket = 0; 
        uint32_t m_PollGroup = 0;
        uint32_t m_Connection = 0;

        std::vector<uint32_t> m_ActiveConnections;

        bool m_IsServer = false;
        bool m_IsClient = false;
        
        std::vector<std::vector<uint8_t>> m_MessageQueue;
        
        static NetworkService* s_Instance;
    };
}

#endif // CH_NETWORK_SERVICE_H

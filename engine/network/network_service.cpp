#include "network_service.h"
#include "engine/core/log.h"
#include "engine/foundation/engine_assert.h"
#include "engine/foundation/timestep.h"


// Steam Networking Sockets Headers
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <algorithm>

namespace Chained
{
    NetworkService* NetworkService::s_Instance = nullptr;

    void NetworkService::Init()
    {
        CH_ASSERT(!s_Instance);
        s_Instance = new NetworkService();
        s_Instance->InitializeSteamNetworking();
    }

    void NetworkService::Shutdown()
    {
        if (s_Instance)
        {
            s_Instance->Disconnect();
            GameNetworkingSockets_Kill();
            delete s_Instance;
            s_Instance = nullptr;
        }
    }

    NetworkService& NetworkService::Get()
    {
        CH_ASSERT(s_Instance);
        return *s_Instance;
    }

    NetworkService::NetworkService()
    {
    }

    NetworkService::~NetworkService()
    {
    }

    void NetworkService::InitializeSteamNetworking()
    {
        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg))
        {
            CH_CORE_ERROR("NetworkService: Unified SteamNetworkingSockets init failed: {}", (const char*)errMsg);
            return;
        }

        CH_CORE_INFO("NetworkService: SteamNetworkingSockets initialized.");
    }

    bool NetworkService::Host(uint16_t port)
    {
        if (IsActive()) Disconnect();

        SteamNetworkingIPAddr serverLocalAddr;
        serverLocalAddr.Clear();
        serverLocalAddr.m_port = port;

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)OnSteamNetConnectionStatusChangedInternal);

        m_ListenSocket = (uint32_t)SteamNetworkingSockets()->CreateListenSocketIP(serverLocalAddr, 1, &opt);
        if (m_ListenSocket == 0)
        {
            CH_CORE_ERROR("NetworkService: Failed to create listen socket on port {}", port);
            return false;
        }

        m_PollGroup = (uint32_t)SteamNetworkingSockets()->CreatePollGroup();
        m_IsServer = true;
        CH_CORE_INFO("NetworkService: Server started on port {}", port);
        return true;
    }

    bool NetworkService::Connect(const char* address)
    {
        if (IsActive()) Disconnect();

        SteamNetworkingIPAddr serverAddr;
        if (!serverAddr.ParseString(address))
        {
            CH_CORE_ERROR("NetworkService: Invalid address string: {}", address);
            return false;
        }

        SteamNetworkingConfigValue_t opt;
        opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)OnSteamNetConnectionStatusChangedInternal);

        m_Connection = (uint32_t)SteamNetworkingSockets()->ConnectByIPAddress(serverAddr, 1, &opt);
        if (m_Connection == 0)
        {
            CH_CORE_ERROR("NetworkService: Failed to initiate connection to {}", address);
            return false;
        }

        m_IsClient = true;
        CH_CORE_INFO("NetworkService: Connecting to {}...", address);
        return true;
    }

    void NetworkService::Disconnect()
    {
        if (m_Connection != 0)
        {
            SteamNetworkingSockets()->CloseConnection((HSteamNetConnection)m_Connection, 0, "Disconnecting", true);
            m_Connection = 0;
        }

        if (m_ListenSocket != 0)
        {
            SteamNetworkingSockets()->CloseListenSocket((HSteamListenSocket)m_ListenSocket);
            m_ListenSocket = 0;
        }

        if (m_PollGroup != 0)
        {
            SteamNetworkingSockets()->DestroyPollGroup((HSteamNetPollGroup)m_PollGroup);
            m_PollGroup = 0;
        }

        m_IsServer = false;
        m_IsClient = false;
        m_MessageQueue.clear();
        m_ActiveConnections.clear();
    }

    void NetworkService::Poll()
    {
        if (!IsActive()) return;

        SteamNetworkingSockets()->RunCallbacks();

        // Poll messages
        constexpr int MAX_MSGS = 16;
        ISteamNetworkingMessage* pIncomingMsgs[MAX_MSGS];
        int numMsgs = 0;
        
        if (m_IsServer && m_PollGroup != 0)
            numMsgs = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup((HSteamNetPollGroup)m_PollGroup, pIncomingMsgs, MAX_MSGS);
        else if (m_Connection != 0)
            numMsgs = SteamNetworkingSockets()->ReceiveMessagesOnConnection((HSteamNetConnection)m_Connection, pIncomingMsgs, MAX_MSGS);

        for (int i = 0; i < numMsgs; ++i)
        {
            auto* pMsg = pIncomingMsgs[i];
            if (pMsg)
            {
                std::vector<uint8_t> data;
                data.assign((uint8_t*)pMsg->GetData(), (uint8_t*)pMsg->GetData() + pMsg->GetSize());
                m_MessageQueue.push_back(std::move(data));
                pMsg->Release();
            }
        }
    }

    bool NetworkService::SendData(const void* data, uint32_t size, bool reliable)
    {
        if (!IsActive()) return false;

        int sendFlags = reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;

        if (m_IsServer)
        {
            return Broadcast(data, size, reliable);
        }
        else if (m_Connection != 0)
        {
            EResult res = SteamNetworkingSockets()->SendMessageToConnection((HSteamNetConnection)m_Connection, data, size, sendFlags, nullptr);
            return res == k_EResultOK;
        }

        return false;
    }

    bool NetworkService::Broadcast(const void* data, uint32_t size, bool reliable)
    {
        if (!m_IsServer) return false;
        if (m_ActiveConnections.empty()) return true;

        int sendFlags = reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
        bool allSuccess = true;

        for (uint32_t conn : m_ActiveConnections)
        {
            EResult res = SteamNetworkingSockets()->SendMessageToConnection((HSteamNetConnection)conn, data, size, sendFlags, nullptr);
            if (res != k_EResultOK) allSuccess = false;
        }

        return allSuccess;
    }

    std::vector<uint8_t> NetworkService::GetNextMessage()
    {
        if (m_MessageQueue.empty()) return {};
        std::vector<uint8_t> msg = std::move(m_MessageQueue.front());
        m_MessageQueue.erase(m_MessageQueue.begin());
        return msg;
    }

    void NetworkService::OnSteamNetConnectionStatusChangedInternal(void* pInfo)
    {
        if (s_Instance)
        {
            s_Instance->HandleConnectionStatusChanged(pInfo);
        }
    }

    void NetworkService::HandleConnectionStatusChanged(void* pInternalInfo)
    {
        auto* pInfo = (SteamNetConnectionStatusChangedCallback_t*)pInternalInfo;
        switch (pInfo->m_info.m_eState)
        {
            case k_ESteamNetworkingConnectionState_None:
                break;
            case k_ESteamNetworkingConnectionState_Connecting:
                if (m_IsServer)
                {
                    CH_CORE_INFO("NetworkService: Incoming connection from client...");
                    SteamNetworkingSockets()->AcceptConnection(pInfo->m_hConn);
                    SteamNetworkingSockets()->SetConnectionPollGroup(pInfo->m_hConn, (HSteamNetPollGroup)m_PollGroup);
                    m_ActiveConnections.push_back((uint32_t)pInfo->m_hConn);
                }
                break;
            case k_ESteamNetworkingConnectionState_Connected:
                CH_CORE_INFO("NetworkService: Connected!");
                break;
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
                CH_CORE_WARN("NetworkService: Connection closed/lost: {}", (const char*)pInfo->m_info.m_szEndDebug);
                SteamNetworkingSockets()->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
                
                if (m_IsServer)
                {
                    auto it = std::ranges::find(m_ActiveConnections, (uint32_t)pInfo->m_hConn);
                    if (it != m_ActiveConnections.end()) m_ActiveConnections.erase(it);
                }
                
                if (pInfo->m_hConn == (HSteamNetConnection)m_Connection) m_Connection = 0;
                break;
            default:
                break;
        }
    }
}

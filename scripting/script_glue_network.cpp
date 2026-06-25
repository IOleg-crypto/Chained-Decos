// #include "scripting/script_glue_internal.h"
// #include "scripting/script_internal_call_registry.h"
// #include "engine/network/network_service.h"

// namespace Chained {

//     namespace
//     {
//         NetworkService* GetNetworkService()
//         {
//             return Engine::GetNetwork();
//         }
//     }

//     // ── Network Service ──────────────────────────────────────────────────
    
//     CH_SCRIPT_FUNC bool Network_Host(uint16_t port) {
//         if (auto* network = GetNetworkService())
//             return network->Host(port);
//         return false;
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_Host_Ptr, Network_Host);

//     CH_SCRIPT_FUNC bool Network_Connect(Coral::String address) {
//         if (auto* network = GetNetworkService())
//             return network->Connect(((std::string)address).c_str());
//         return false;
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_Connect_Ptr, Network_Connect);

//     CH_SCRIPT_FUNC void Network_Disconnect() {
//         if (auto* network = GetNetworkService())
//             network->Disconnect();
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_Disconnect_Ptr, Network_Disconnect);

//     CH_SCRIPT_FUNC bool Network_IsActive() {
//         if (auto* network = GetNetworkService())
//             return network->IsActive();
//         return false;
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_IsActive_Ptr, Network_IsActive);

//     CH_SCRIPT_FUNC bool Network_IsServer() {
//         if (auto* network = GetNetworkService())
//             return network->IsServer();
//         return false;
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_IsServer_Ptr, Network_IsServer);

//     CH_SCRIPT_FUNC bool Network_SendData(uint8_t* data, uint32_t size, bool reliable) {
//         if (auto* network = GetNetworkService())
//             return network->SendData(data, size, reliable);
//         return false;
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_SendData_Ptr, Network_SendData);

//     CH_SCRIPT_FUNC bool Network_HasMessages() {
//         if (auto* network = GetNetworkService())
//             return network->HasMessages();
//         return false;
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_HasMessages_Ptr, Network_HasMessages);

//     CH_SCRIPT_FUNC Coral::Array<uint8_t> Network_GetNextMessage() {
//         if (auto* network = GetNetworkService()) {
//             auto msg = network->GetNextMessage();
//             if (msg.empty()) return Coral::Array<uint8_t>();
            
//             auto arr = Coral::Array<uint8_t>::New(msg.size());
//             for (size_t i = 0; i < msg.size(); ++i) arr[i] = msg[i];
//             return arr;
//         }
//         return Coral::Array<uint8_t>();
//     }
//     CH_ADD_INTERNAL_CALL(Network, Network_GetNextMessage_Ptr, Network_GetNextMessage);

//     void RegisterGlueNetwork() {
//         // Mappings are already added via CH_ADD_INTERNAL_CALL static initializers
//     }

// } // namespace Chained

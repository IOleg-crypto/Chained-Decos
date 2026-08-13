#ifndef CH_NETWORK_SYSTEM_H
#define CH_NETWORK_SYSTEM_H

#include "engine/common/timestep.h"
#include "engine/common/uuid.h"
#include "engine/networking/net_packet.h"
#include "engine/networking/network_service.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace Chained
{
	class Scene;

	// NetworkPeerHandle and Role are now defined in network_service.h

	struct PendingNetworkState
	{
		uint64_t NetworkID = 0;
		glm::vec3 TargetPosition = {0, 0, 0};
		glm::quat TargetRotation = {1, 0, 0, 0};
		glm::vec3 TargetVelocity = {0, 0, 0};
	};

	struct ProcessedInput
	{
		uint64_t NetworkID = 0;
		float MoveX = 0.0f;
		float MoveZ = 0.0f;
		uint8_t ActionFlags = 0;
		float MouseX = 0.0f;
		float MouseY = 0.0f;
	};

	class NetworkSystem
	{
	public:
		static NetworkSystem& GetInstance();

		// Maximum distance (m) between local and server position before snap-correction
		// kicks in for owned entities. Below this threshold, local physics runs freely.
		static constexpr float kMaxCorrectionDistance = 2.0f;

		// Ensure the local player entity has a NetworkIdentityComponent BEFORE scripts
		// run. Must be called early in the frame so PlayerController etc. can find it.
		void EnsureLocalIdentity(Scene* scene);

		void Update(Scene* scene, Timestep ts);
		void ApplyHostInputs(entt::registry& reg, Timestep ts);

		const std::vector<ProcessedInput>& GetPendingInputs();
		void ClearPendingInputs();

		void RegisterPeerEntity(int peer, uint64_t networkID);
		void UnregisterPeer(int peer);

		// Prefab instantiated for each connecting client. Relative to the assets
		// directory. Empty by default — clients get no avatar until this is set.
		void SetPlayerPrefab(const std::string& path);
		const std::string& GetPlayerPrefab();

		// Chat messages received from the network (for C# consumption).
		const std::vector<ChatMessagePacket>& GetPendingChatMessages();
		void ClearPendingChatMessages();

	private:
		NetworkSystem() = default;

		// ---- Incoming message processing ----
		void ProcessWorldStateMessage(WorldStateMessage* msg);
		void ProcessSceneChangeMessage(SceneChangeMessage* msg);
		void ProcessEntitySpawnMessage(EntitySpawnMessage* msg, Scene* scene);
		void ProcessEntityDestroyMessage(EntityDestroyMessage* msg, Scene* scene);
		void ProcessPlayerAssignMessage(PlayerAssignMessage* msg);

		// ---- Incoming message processing (host side) ----
		void ProcessInputStateMessage(InputStateMessage* msg, int clientIndex);
		void ProcessPlayerInfoMessage(PlayerInfoMessage* msg, int clientIndex);
		void ProcessPlayerListMessage(PlayerListMessage* msg);
		void ProcessChatMessageMessage(Chained::ChatMessageMessage* msg);

		// ---- Host-side helpers ----
		void EnsureHostIdentity(Scene* scene);
		void SyncPeerAvatars(Scene* scene, Network* net);
		void BroadcastWorldState(entt::registry& reg);
		void SendEntitySpawn(Network* net, uint64_t networkID, const std::string& prefabPath, int clientIndex);
		void SendEntityDestroy(Network* net, uint64_t networkID);

		// ---- Client-side helpers ----
		void CollectAndSendInput(Network* net, float dt);
		void InterpolateEntities(entt::registry& reg, float dt);

		// ---- Packet callback installer ----
		void InstallPacketCallback();

		// ---- Member state (previously file-scope globals) ----
		std::unordered_map<uint64_t, PendingNetworkState> m_PendingStates;
		std::vector<ProcessedInput> m_PendingInputs;
		uint32_t m_ClientTick = 0;
		std::unordered_map<int, uint64_t> m_PeerToNetworkID;
		Role m_CallbackRole = Role::Offline;
		std::string m_PlayerPrefab = "prefab/player.chprefab";
		std::unordered_map<int, UUID> m_PeerToAvatar;
		std::vector<ChatMessagePacket> m_PendingChatMessages;
		std::unordered_map<uint64_t, UUID> m_NetworkIDToEntity;
		uint64_t m_LocalNetworkID = 0;
		Scene* m_ReplicationScene = nullptr;

		static constexpr uint64_t HostAvatarNetworkID = 1;
	};

} // namespace Chained

#endif // CH_NETWORK_SYSTEM_H

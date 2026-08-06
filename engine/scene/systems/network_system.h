#ifndef CH_NETWORK_SYSTEM_H
#define CH_NETWORK_SYSTEM_H

#include "engine/common/timestep.h"
#include "engine/networking/net_packet.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

#include <steam/steamnetworkingsockets.h>

namespace Chained
{
	class Scene;

	struct PendingNetworkState
	{
		uint64_t NetworkID = 0;
		glm::vec3 TargetPosition = {0, 0, 0};
		glm::quat TargetRotation = {1, 0, 0, 0};
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

	namespace NetworkSystem
	{
		void Update(Scene* scene, Timestep ts);
		void ApplyHostInputs(entt::registry& reg, Timestep ts);

		const std::vector<ProcessedInput>& GetPendingInputs();
		void ClearPendingInputs();

		void RegisterPeerEntity(HSteamNetConnection peer, uint64_t networkID);
		void UnregisterPeer(HSteamNetConnection peer);

		// Prefab instantiated for each connecting client. Relative to the assets
		// directory. Empty by default — clients get no avatar until this is set.
		void SetPlayerPrefab(const std::string& path);
		const std::string& GetPlayerPrefab();

		// Chat messages received from the network (for C# consumption).
		const std::vector<ChatMessagePacket>& GetPendingChatMessages();
		void ClearPendingChatMessages();
	} // namespace NetworkSystem

} // namespace Chained

#endif // CH_NETWORK_SYSTEM_H

#ifndef CH_NETWORK_SYSTEM_H
#define CH_NETWORK_SYSTEM_H

#include "engine/common/timestep.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct _ENetPeer;
typedef struct _ENetPeer ENetPeer;

namespace Chained
{

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
		void Update(entt::registry& reg, Timestep ts);
		void ApplyHostInputs(entt::registry& reg, Timestep ts);

		const std::vector<ProcessedInput>& GetPendingInputs();
		void ClearPendingInputs();

		void RegisterPeerEntity(ENetPeer* peer, uint64_t networkID);
		void UnregisterPeer(ENetPeer* peer);
	} // namespace NetworkSystem

} // namespace Chained

#endif // CH_NETWORK_SYSTEM_H

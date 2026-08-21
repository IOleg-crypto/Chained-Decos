#ifndef CH_NETWORK_IDENTITY_COMPONENT_H
#define CH_NETWORK_IDENTITY_COMPONENT_H

#include "engine/scene/component_registry.h"
#include <cstdint>
#include <string>

namespace Chained
{
	struct NetworkIdentityComponent
	{
		uint64_t NetworkID = 0;
		bool IsOwner = true;
		std::string PrefabPath;		   // track which prefab was used to spawn this entity
		uint8_t RemoteActionFlags = 0; // replicated from host (sprint/jump state)

		static const char* GetStaticName()
		{
			return "NetworkIdentityComponent";
		}

		struct UI
		{
			UIMeta NetworkID = {.ReadOnly = true, .Tooltip = "Unique network entity ID"};
			UIMeta IsOwner = {.Tooltip = "Whether this client owns this entity"};
			UIMeta PrefabPath = {.Tooltip = "Prefab asset path used to spawn"};
			UIMeta RemoteActionFlags = {.ReadOnly = true, .Tooltip = "Replicated action flags (sprint/jump)"};
		};
	};
	CH_MARK_RFL(NetworkIdentityComponent);
} // namespace Chained

#endif // CH_NETWORK_IDENTITY_COMPONENT_H

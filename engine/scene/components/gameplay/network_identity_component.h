#ifndef CH_NETWORK_IDENTITY_COMPONENT_H
#define CH_NETWORK_IDENTITY_COMPONENT_H

#include "engine/scene/component_registry.h"
#include <cstdint>

namespace Chained
{
	struct NetworkIdentityComponent
	{
		uint64_t NetworkID = 0;
		bool IsOwner = true;

		static const char* GetStaticName()
		{
			return "NetworkIdentityComponent";
		}

		struct UI
		{
			UIMeta NetworkID = {.Tooltip = "Unique network entity ID", .ReadOnly = true};
			UIMeta IsOwner = {.Tooltip = "Whether this client owns this entity"};
		};
	};
	CH_MARK_RFL(NetworkIdentityComponent);
} // namespace Chained

#endif // CH_NETWORK_IDENTITY_COMPONENT_H

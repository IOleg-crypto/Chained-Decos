#ifndef CH_SCENE_STATE_H
#define CH_SCENE_STATE_H

#include <cstdint>

namespace Chained
{

	enum class SceneState : uint8_t
	{
		None = 0,
		Edit = 1,
		Play = 2,
		Simulate = 3
	};

} // namespace Chained

#endif // CH_SCENE_STATE_H

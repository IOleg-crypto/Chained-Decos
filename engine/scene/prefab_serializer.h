#ifndef CH_PREFAB_SERIALIZER_H
#define CH_PREFAB_SERIALIZER_H

#include "engine/scene/scene.h"
#include <string>
#include <future>

namespace Chained
{
	class PrefabSerializer
	{
	public:
		static bool Serialize(Entity entity, const std::string& filepath);
		static Entity Deserialize(Scene* scene, const std::string& filepath);
		static std::future<Entity> LoadAsync(Scene* scene, const std::string& filepath);
	};

} // namespace Chained

#endif // CH_PREFAB_SERIALIZER_H

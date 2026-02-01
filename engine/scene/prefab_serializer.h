#ifndef CH_PREFAB_SERIALIZER_H
#define CH_PREFAB_SERIALIZER_H

#include "engine/scene/entity.h"
#include <string>

namespace CHEngine
{
    class PrefabSerializer
    {
    public:
        static bool Serialize(Entity entity, const std::string& filepath);
        static Entity Deserialize(Scene* scene, const std::string& filepath);
    };
}

#endif // CH_PREFAB_SERIALIZER_H

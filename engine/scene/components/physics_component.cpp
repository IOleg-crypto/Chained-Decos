#include "engine/scene/components/physics_component.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{
    BoundingBox ColliderComponent::CalculateWorldAABB(const ColliderComponent& collider, const glm::mat4& worldTransform)
    {
        glm::vec3 halfExtents = collider.Size * 0.5f;
        glm::vec3 min = collider.Offset - halfExtents;
        glm::vec3 max = collider.Offset + halfExtents;
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z}, {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };
        BoundingBox result = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        for (int i = 0; i < 8; i++)
        {
            glm::vec3 worldCorner = glm::vec3(worldTransform * glm::vec4(corners[i], 1.0f));
            result.Min = glm::min(result.Min, worldCorner);
            result.Max = glm::max(result.Max, worldCorner);
        }
        return result;
    }
}

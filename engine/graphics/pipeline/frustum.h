#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <glm/glm.hpp>
#include <array>
#include <cmath>

namespace CHEngine
{
struct Frustum
{
    static constexpr size_t PlaneCount = 6;

    enum PlaneIndex : size_t { Left = 0, Right, Bottom, Top, Near, Far };

    // Planes: x, y, z = normal, w = distance from origin
    std::array<glm::vec4, PlaneCount> Planes;

    // Extracts planes from the view-projection matrix.
    // GLM uses column-major storage/access.
    void Extract(const glm::mat4& mat)
    {
        // Gribb-Hartmann plane extraction for column-major matrix
        Planes[Left]   = { mat[0][3] + mat[0][0], mat[1][3] + mat[1][0], mat[2][3] + mat[2][0], mat[3][3] + mat[3][0] };
        Planes[Right]  = { mat[0][3] - mat[0][0], mat[1][3] - mat[1][0], mat[2][3] - mat[2][0], mat[3][3] - mat[3][0] };
        Planes[Bottom] = { mat[0][3] + mat[0][1], mat[1][3] + mat[1][1], mat[2][3] + mat[2][1], mat[3][3] + mat[3][1] };
        Planes[Top]    = { mat[0][3] - mat[0][1], mat[1][3] - mat[1][1], mat[2][3] - mat[2][1], mat[3][3] - mat[3][1] };
        Planes[Near]   = { mat[0][3] + mat[0][2], mat[1][3] + mat[1][2], mat[2][3] + mat[2][2], mat[3][3] + mat[3][2] };
        Planes[Far]    = { mat[0][3] - mat[0][2], mat[1][3] - mat[1][2], mat[2][3] - mat[2][2], mat[3][3] - mat[3][2] };

        // Normalize planes for accurate sphere testing
        for (auto& p : Planes)
        {
            float length = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
            if (length > 0) p /= length;
        }
    }

    // Fast AABB check using center and extents.
    bool IsBoxVisible(const glm::vec3& center, const glm::vec3& extents) const
    {
        for (const auto& p : Planes)
        {
            // Projection of extents onto plane normal
            float r = extents.x * std::abs(p.x) + extents.y * std::abs(p.y) + extents.z * std::abs(p.z);
            // Distance from box center to plane
            float d = p.x * center.x + p.y * center.y + p.z * center.z + p.w;

            if (d < -r) return false;
        }
        return true;
    }

    // Optimized AABB check with transform.
    bool IsBoxVisible(const struct BoundingBox& box, const glm::mat4& transform) const
    {
        // 1. Find local center and extents
        glm::vec3 localCenter = { (box.Max.x + box.Min.x) * 0.5f, (box.Max.y + box.Min.y) * 0.5f, (box.Max.z + box.Min.z) * 0.5f };
        glm::vec3 localExtents = { (box.Max.x - box.Min.x) * 0.5f, (box.Max.y - box.Min.y) * 0.5f, (box.Max.z - box.Min.z) * 0.5f };

        // 2. Transform center to world space
        glm::vec4 worldCenter4 = transform * glm::vec4(localCenter, 1.0f);
        glm::vec3 worldCenter = glm::vec3(worldCenter4);

        // 3. Calculate new extents in world space (taking rotation and scale into account)
        glm::vec3 worldExtents = {
            std::abs(transform[0][0]) * localExtents.x + std::abs(transform[1][0]) * localExtents.y + std::abs(transform[2][0]) * localExtents.z,
            std::abs(transform[0][1]) * localExtents.x + std::abs(transform[1][1]) * localExtents.y + std::abs(transform[2][1]) * localExtents.z,
            std::abs(transform[0][2]) * localExtents.x + std::abs(transform[1][2]) * localExtents.y + std::abs(transform[2][2]) * localExtents.z
        };

        return IsBoxVisible(worldCenter, worldExtents);
    }

    bool IsSphereVisible(const glm::vec3& center, float radius) const
    {
        for (const auto& p : Planes)
        {
            if (p.x * center.x + p.y * center.y + p.z * center.z + p.w < -radius) return false;
        }
        return true;
    }
};
} // namespace CHEngine

#endif
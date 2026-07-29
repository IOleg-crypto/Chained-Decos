#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <glm/glm.hpp>

namespace Chained
{
// Frustum plane: xyz = normal (normalized), w = distance from origin.
// Point is inside if dot(normal, point) + w >= 0.
struct Frustum
{
    glm::vec4 Left, Right, Bottom, Top, Near, Far;
};

inline glm::vec4 NormalizePlane(const glm::vec4& p)
{
    float len = glm::length(glm::vec3(p));
    // If length is 0 (shouldn't happen with valid VP matrix), avoid division by zero
    if (len < 0.0001f)
    {
        return p;
    }
    return p / len;
}

inline Frustum FromMatrix(const glm::mat4& vp)
{
    Frustum f;
    f.Left =
        NormalizePlane(glm::vec4{vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]});
    f.Right =
        NormalizePlane(glm::vec4{vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]});
    f.Bottom =
        NormalizePlane(glm::vec4{vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]});
    f.Top =
        NormalizePlane(glm::vec4{vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]});
#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
    f.Near = NormalizePlane(glm::vec4{vp[0][2], vp[1][2], vp[2][2], vp[3][2]});
#else
    f.Near =
        NormalizePlane(glm::vec4{vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]});
#endif
    f.Far =
        NormalizePlane(glm::vec4{vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]});
    return f;
}

inline bool IsSphereVisible(const Frustum& frustum, const glm::vec3& center, float radius)
{
    auto outside = [&](const glm::vec4& p) { return glm::dot(glm::vec3(p), center) + p.w < -radius; };
    return !outside(frustum.Left) && !outside(frustum.Right) && !outside(frustum.Bottom) && !outside(frustum.Top) &&
           !outside(frustum.Near) && !outside(frustum.Far);
}

inline bool IsBoxVisible(const Frustum& frustum, const glm::vec3& center, const glm::vec3& extents)
{
    auto outside = [&](const glm::vec4& p) {
        float r = glm::dot(glm::abs(glm::vec3(p)), extents);
        return glm::dot(glm::vec3(p), center) + p.w < -r;
    };
    return !outside(frustum.Left) && !outside(frustum.Right) && !outside(frustum.Bottom) && !outside(frustum.Top) &&
           !outside(frustum.Near) && !outside(frustum.Far);
}
} // namespace Chained

#endif // CH_FRUSTUM_H
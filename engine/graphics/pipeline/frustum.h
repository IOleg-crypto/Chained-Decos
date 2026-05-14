#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <glm/glm.hpp>

namespace CHEngine
{
// Frustum plane: xyz = normal (normalized), w = distance from origin.
// Point is inside if dot(normal, point) + w >= 0.
struct Frustum
{
    glm::vec4 Left, Right, Bottom, Top, Near, Far;
};

static Frustum FromMatrix(const glm::mat4& vp)
{
    Frustum f;
    f.Left =
        glm::normalize(glm::vec4{vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]});
    f.Right =
        glm::normalize(glm::vec4{vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]});
    f.Bottom =
        glm::normalize(glm::vec4{vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]});
    f.Top =
        glm::normalize(glm::vec4{vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]});
    f.Near =
        glm::normalize(glm::vec4{vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]});
    f.Far =
        glm::normalize(glm::vec4{vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]});
    return f;
}

inline bool IsSphereVisible(Frustum frustum, const glm::vec3& center, float radius)
{
    auto outside = [&](const glm::vec4& p) { return glm::dot(glm::vec3(p), center) + p.w < -radius; };
    return !outside(frustum.Left) && !outside(frustum.Right) && !outside(frustum.Bottom) && !outside(frustum.Top) &&
           !outside(frustum.Near) && !outside(frustum.Far);
}

inline bool IsBoxVisible(Frustum frustum, const glm::vec3& center, const glm::vec3& extents)
{
    auto outside = [&](const glm::vec4& p) {
        float r = glm::dot(glm::abs(glm::vec3(p)), extents);
        return glm::dot(glm::vec3(p), center) + p.w < -r;
    };
    return !outside(frustum.Left) && !outside(frustum.Right) && !outside(frustum.Bottom) && !outside(frustum.Top) &&
           !outside(frustum.Near) && !outside(frustum.Far);
}
} // namespace CHEngine

#endif // CH_FRUSTUM_H
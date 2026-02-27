#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <raylib.h>
#include <raymath.h>
#include <cmath>

namespace CHEngine
{
struct Frustum
{
    // Planes stored as Vector4 (x, y, z = normal, w = distance)
    Vector4 Planes[6];

    /**
     * Extracts frustum planes from a view-projection matrix.
     * Order: Left, Right, Bottom, Top, Near, Far.
     */
    void Extract(Matrix mat)
    {
        // Gribb-Hartmann method for column-major matrices (standard in Raylib/OpenGL)
        Planes[0] = {mat.m3 + mat.m0, mat.m7 + mat.m4, mat.m11 + mat.m8, mat.m15 + mat.m12};  // Left
        Planes[1] = {mat.m3 - mat.m0, mat.m7 - mat.m4, mat.m11 - mat.m8, mat.m15 - mat.m12};  // Right
        Planes[2] = {mat.m3 + mat.m1, mat.m7 + mat.m5, mat.m11 + mat.m9, mat.m15 + mat.m13};  // Bottom
        Planes[3] = {mat.m3 - mat.m1, mat.m7 - mat.m5, mat.m11 - mat.m9, mat.m15 - mat.m13};  // Top
        Planes[4] = {mat.m3 + mat.m2, mat.m7 + mat.m6, mat.m11 + mat.m10, mat.m15 + mat.m14}; // Near
        Planes[5] = {mat.m3 - mat.m2, mat.m7 - mat.m6, mat.m11 - mat.m10, mat.m15 - mat.m14}; // Far

        // Normalize planes for accurate distance-based visibility checks
        for (int i = 0; i < 6; i++)
        {
            float len = sqrtf(Planes[i].x * Planes[i].x + Planes[i].y * Planes[i].y + Planes[i].z * Planes[i].z);
            float invLen = (len != 0.0f) ? 1.0f / len : 1.0f;
            Planes[i].x *= invLen;
            Planes[i].y *= invLen;
            Planes[i].z *= invLen;
            Planes[i].w *= invLen;
        }
    }

    /**
     * Checks if a BoundingBox (in world space) is visible.
     */
    bool IsBoxVisible(BoundingBox box) const
    {
        Vector3 center = {(box.max.x + box.min.x) * 0.5f, (box.max.y + box.min.y) * 0.5f,
                          (box.max.z + box.min.z) * 0.5f};
        Vector3 extents = {(box.max.x - box.min.x) * 0.5f, (box.max.y - box.min.y) * 0.5f,
                           (box.max.z - box.min.z) * 0.5f};

        for (int i = 0; i < 6; i++)
        {
            float r = extents.x * fabsf(Planes[i].x) + extents.y * fabsf(Planes[i].y) + extents.z * fabsf(Planes[i].z);
            float d = Planes[i].x * center.x + Planes[i].y * center.y + Planes[i].z * center.z + Planes[i].w;

            if (d < -r)
            {
                return false;
            }
        }
        return true;
    }

    /**
     * Checks if a BoundingBox (in local space) is visible after transformation.
     */
    bool IsBoxVisible(BoundingBox box, Matrix transform) const
    {
        // Compute center and extents in local space
        Vector3 c = {(box.max.x + box.min.x) * 0.5f, (box.max.y + box.min.y) * 0.5f, (box.max.z + box.min.z) * 0.5f};
        Vector3 e = {(box.max.x - box.min.x) * 0.5f, (box.max.y - box.min.y) * 0.5f, (box.max.z - box.min.z) * 0.5f};

        // Transform center to world space
        Vector3 centerWorld = Vector3Transform(c, transform);

        for (int i = 0; i < 6; i++)
        {
            // Project world-space axes on the plane normal to compute the effective radius
            // We use the absolute values of the transformed axes
            float r = e.x * fabsf(Planes[i].x * transform.m0 + Planes[i].y * transform.m1 + Planes[i].z * transform.m2) +
                      e.y * fabsf(Planes[i].x * transform.m4 + Planes[i].y * transform.m5 + Planes[i].z * transform.m6) +
                      e.z * fabsf(Planes[i].x * transform.m8 + Planes[i].y * transform.m9 + Planes[i].z * transform.m10);

            // Distance from world-space center to plane
            float d = Planes[i].x * centerWorld.x + Planes[i].y * centerWorld.y + Planes[i].z * centerWorld.z + Planes[i].w;

            if (d < -r)
            {
                return false;
            }
        }
        return true;
    }

    /**
     * Checks if a sphere is visible within the frustum.
     */
    bool IsSphereVisible(Vector3 center, float radius) const
    {
        for (int i = 0; i < 6; i++)
        {
            float d = Planes[i].x * center.x + Planes[i].y * center.y + Planes[i].z * center.z + Planes[i].w;
            if (d < -radius)
            {
                return false;
            }
        }
        return true;
    }
};
} // namespace CHEngine

#endif // CH_FRUSTUM_H

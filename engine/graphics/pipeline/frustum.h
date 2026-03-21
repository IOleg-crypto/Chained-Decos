#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <raylib.h>
#include <raymath.h>
#include <array>
#include <cstddef>
#include <cmath>

namespace CHEngine
{
/**
 * Frustum class for view frustum culling.
 * Uses Gribb-Hartmann method for plane extraction from a View-Projection matrix.
 */
struct Frustum
{
    static constexpr size_t PlaneCount = 6;
    static constexpr size_t CornerCount = 8;

    enum PlaneIndex : size_t
    {
        Left = 0, Right, Bottom, Top, Near, Far
    };

    // Planes stored as Vector4 (x, y, z = normal, w = distance)
    // The normal (x,y,z) points INSIDE the frustum.
    std::array<Vector4, PlaneCount> Planes;

    /**
     * Extracts frustum planes from a View-Projection matrix (P * V).
     */
    void Extract(Matrix mat)
    {
        // Extracting rows for cleaner math (Raylib struct names m0..m15 map to memory col-major)
        // Row 1: m0 m4 m8 m12
        // Row 2: m1 m5 m9 m13
        // Row 3: m2 m6 m10 m14
        // Row 4: m3 m7 m11 m15

        // Left:   col4 + col1
        Planes[Left]   = { mat.m3 + mat.m0, mat.m7 + mat.m4, mat.m11 + mat.m8, mat.m15 + mat.m12 };
        // Right:  col4 - col1
        Planes[Right]  = { mat.m3 - mat.m0, mat.m7 - mat.m4, mat.m11 - mat.m8, mat.m15 - mat.m12 };
        // Bottom: col4 + col2
        Planes[Bottom] = { mat.m3 + mat.m1, mat.m7 + mat.m5, mat.m11 + mat.m9, mat.m15 + mat.m13 };
        // Top:    col4 - col2
        Planes[Top]    = { mat.m3 - mat.m1, mat.m7 - mat.m5, mat.m11 - mat.m9, mat.m15 - mat.m13 };
        // Near:   col4 + col3
        Planes[Near]   = { mat.m3 + mat.m2, mat.m7 + mat.m6, mat.m11 + mat.m10, mat.m15 + mat.m14 };
        // Far:    col4 - col3
        Planes[Far]    = { mat.m3 - mat.m2, mat.m7 - mat.m6, mat.m11 - mat.m10, mat.m15 - mat.m14 };

        // Normalize planes for accurate distance-based visibility checks
        for (auto& plane : Planes)
        {
            float length = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
            float invLength = (length != 0.0f) ? 1.0f / length : 1.0f;
            plane.x *= invLength;
            plane.y *= invLength;
            plane.z *= invLength;
            plane.w *= invLength;
        }
    }

    /**
     * Checks if a BoundingBox (in world space) is visible.
     */
    bool IsBoxVisible(BoundingBox box) const
    {
        Vector3 center = { (box.max.x + box.min.x) * 0.5f, (box.max.y + box.min.y) * 0.5f, (box.max.z + box.min.z) * 0.5f };
        Vector3 extents = { (box.max.x - box.min.x) * 0.5f, (box.max.y - box.min.y) * 0.5f, (box.max.z - box.min.z) * 0.5f };

        return IsBoxVisible(center, extents);
    }

    /**
     * Checks if a BoundingBox (centered at 'center' with 'extents') is visible.
     * Pure mathematical center-extents method (most efficient for axis-aligned checks).
     */
    bool IsBoxVisible(Vector3 center, Vector3 extents) const
    {
        for (const auto& plane : Planes)
        {
            // Effective radius of the box projected onto the plane normal
            float r = extents.x * fabsf(plane.x) + extents.y * fabsf(plane.y) + extents.z * fabsf(plane.z);
            // Distance from plane to box center
            float d = plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w;

            // If the whole box (center-radius) is behind the plane, it is culled
            if (d < -r) return false;
        }
        return true;
    }

    /**
     * Checks if a BoundingBox (in local space) is visible after transformation.
     */
    bool IsBoxVisible(BoundingBox box, Matrix transform) const
    {
        // Transform the 8 corners of the AABB to world space
        // This is 100% robust against rotation and scale
        std::array<Vector3, CornerCount> corners = {
            Vector3Transform({ box.min.x, box.min.y, box.min.z }, transform),
            Vector3Transform({ box.min.x, box.min.y, box.max.z }, transform),
            Vector3Transform({ box.min.x, box.max.y, box.min.z }, transform),
            Vector3Transform({ box.min.x, box.max.y, box.max.z }, transform),
            Vector3Transform({ box.max.x, box.min.y, box.min.z }, transform),
            Vector3Transform({ box.max.x, box.min.y, box.max.z }, transform),
            Vector3Transform({ box.max.x, box.max.y, box.min.z }, transform),
            Vector3Transform({ box.max.x, box.max.y, box.max.z }, transform)
        };

        for (const auto& plane : Planes)
        {
            size_t outCount = 0;
            for (const auto& corner : corners)
            {
                float dist = plane.x * corner.x + plane.y * corner.y + plane.z * corner.z + plane.w;
                if (dist < 0) outCount++;
            }
            // If all corners are behind a single plane, the object is outside
            if (outCount == corners.size()) return false;
        }

        return true;
    }

    /**
     * Checks if a sphere is visible within the frustum.
     */
    bool IsSphereVisible(Vector3 center, float radius) const
    {
        for (const auto& plane : Planes)
        {
            float d = plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w;
            if (d < -radius) return false;
        }
        return true;
    }
};
} // namespace CHEngine

#endif // CH_FRUSTUM_H

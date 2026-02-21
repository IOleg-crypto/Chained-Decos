#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <raylib.h>
#include <raymath.h>

namespace CHEngine
{
struct Plane
{
    Vector3 Normal;
    float Distance;

    float DistanceToPoint(Vector3 point) const
    {
        return Vector3DotProduct(Normal, point) + Distance;
    }
};

struct Frustum
{
    Plane Planes[6];

    void Extract(Matrix viewProjection)
    {
        // Gribb-Hartmann method for extracting frustum planes from view-projection matrix

        // Left
        Planes[0].Normal.x = viewProjection.m3 + viewProjection.m0;
        Planes[0].Normal.y = viewProjection.m7 + viewProjection.m4;
        Planes[0].Normal.z = viewProjection.m11 + viewProjection.m8;
        Planes[0].Distance = viewProjection.m15 + viewProjection.m12;

        // Right
        Planes[1].Normal.x = viewProjection.m3 - viewProjection.m0;
        Planes[1].Normal.y = viewProjection.m7 - viewProjection.m4;
        Planes[1].Normal.z = viewProjection.m11 - viewProjection.m8;
        Planes[1].Distance = viewProjection.m15 - viewProjection.m12;

        // Bottom
        Planes[2].Normal.x = viewProjection.m3 + viewProjection.m1;
        Planes[2].Normal.y = viewProjection.m7 + viewProjection.m5;
        Planes[2].Normal.z = viewProjection.m11 + viewProjection.m9;
        Planes[2].Distance = viewProjection.m15 + viewProjection.m13;

        // Top
        Planes[3].Normal.x = viewProjection.m3 - viewProjection.m1;
        Planes[3].Normal.y = viewProjection.m7 - viewProjection.m5;
        Planes[3].Normal.z = viewProjection.m11 - viewProjection.m9;
        Planes[3].Distance = viewProjection.m15 - viewProjection.m13;

        // Near
        Planes[4].Normal.x = viewProjection.m3 + viewProjection.m2;
        Planes[4].Normal.y = viewProjection.m7 + viewProjection.m6;
        Planes[4].Normal.z = viewProjection.m11 + viewProjection.m10;
        Planes[4].Distance = viewProjection.m15 + viewProjection.m14;

        // Far
        Planes[5].Normal.x = viewProjection.m3 - viewProjection.m2;
        Planes[5].Normal.y = viewProjection.m7 - viewProjection.m6;
        Planes[5].Normal.z = viewProjection.m11 - viewProjection.m10;
        Planes[5].Distance = viewProjection.m15 - viewProjection.m14;

        for (int i = 0; i < 6; i++)
        {
            float length = Vector3Length(Planes[i].Normal);
            Planes[i].Normal = Vector3Scale(Planes[i].Normal, 1.0f / (length > 0 ? length : 1.0f));
            Planes[i].Distance /= (length > 0 ? length : 1.0f);
        }
    }

    bool IsBoxVisible(BoundingBox box, Matrix transform) const
    {
        // Transform BoundingBox corners (Center + Extents approach is faster but corners are robust for skewed
        // transforms)
        Vector3 corners[8] = {{box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
                              {box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
                              {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
                              {box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z}};

        for (int i = 0; i < 8; i++)
        {
            corners[i] = Vector3Transform(corners[i], transform);
        }

        for (int i = 0; i < 6; i++)
        {
            int outCount = 0;
            for (int j = 0; j < 8; j++)
            {
                if (Planes[i].DistanceToPoint(corners[j]) < 0)
                {
                    outCount++;
                }
            }

            // If all corners are outside this plane, the whole box is outside the frustum
            if (outCount == 8)
            {
                return false;
            }
        }
        return true;
    }
};
} // namespace CHEngine

#endif

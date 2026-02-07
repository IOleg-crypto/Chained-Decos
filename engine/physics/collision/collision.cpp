#include "collision.h"
#include <algorithm>
#include <cmath>

namespace CHEngine
{

    Vector3 Collision::GetClosestPointOnTriangle(const Vector3& targetPoint, 
                                                 const CollisionTriangle& triangle)
    {
        Vector3 edge0 = Vector3Subtract(triangle.v1, triangle.v0);
        Vector3 edge1 = Vector3Subtract(triangle.v2, triangle.v0);
        Vector3 vertex0ToPoint = Vector3Subtract(targetPoint, triangle.v0);

        float dot00 = Vector3DotProduct(edge0, edge0);
        float dot01 = Vector3DotProduct(edge0, edge1);
        float dot11 = Vector3DotProduct(edge1, edge1);
        float dot0p = Vector3DotProduct(edge0, vertex0ToPoint);
        float dot1p = Vector3DotProduct(edge1, vertex0ToPoint);

        float barycentricV = dot0p * dot11 - dot01 * dot1p;
        float barycentricW = dot00 * dot1p - dot0p * dot01;
        float determinant = dot00 * dot11 - dot01 * dot01;

        if (barycentricV + barycentricW <= determinant)
        {
            if (barycentricV < 0.0f)
            {
                if (barycentricW < 0.0f)
                {
                    // Region 4
                    if (dot0p < 0.0f)
                    {
                        barycentricW = 0.0f;
                        if (-dot0p >= dot00) barycentricV = 1.0f;
                        else barycentricV = -dot0p / dot00;
                    }
                    else
                    {
                        barycentricV = 0.0f;
                        if (dot1p >= 0.0f) barycentricW = 0.0f;
                        else if (-dot1p >= dot11) barycentricW = 1.0f;
                        else barycentricW = -dot1p / dot11;
                    }
                }
                else
                {
                    // Region 3
                    barycentricV = 0.0f;
                    if (dot1p >= 0.0f) barycentricW = 0.0f;
                    else if (-dot1p >= dot11) barycentricW = 1.0f;
                    else barycentricW = -dot1p / dot11;
                }
            }
            else if (barycentricW < 0.0f)
            {
                // Region 5
                barycentricW = 0.0f;
                if (dot0p >= 0.0f) barycentricV = 0.0f;
                else if (-dot0p >= dot00) barycentricV = 1.0f;
                else barycentricV = -dot0p / dot00;
            }
            else
            {
                // Region 0: Inside triangle
                float inverseDeterminant = 1.0f / determinant;
                barycentricV *= inverseDeterminant;
                barycentricW *= inverseDeterminant;
            }
        }
        else
        {
            if (barycentricV < 0.0f)
            {
                // Region 2
                float temp0 = dot01 + dot0p;
                float temp1 = dot11 + dot1p;
                if (temp1 > temp0)
                {
                    float numerator = temp1 - temp0;
                    float denominator = dot00 - 2.0f * dot01 + dot11;
                    if (numerator >= denominator) barycentricV = 1.0f;
                    else barycentricV = numerator / denominator;
                    barycentricW = 1.0f - barycentricV;
                }
                else
                {
                    barycentricV = 0.0f;
                    if (temp1 >= 0.0f) barycentricW = 0.0f;
                    else if (-temp1 >= dot11) barycentricW = 1.0f;
                    else barycentricW = -temp1 / dot11;
                }
            }
            else if (barycentricW < 0.0f)
            {
                // Region 6
                float temp0 = dot01 + dot1p;
                float temp1 = dot00 + dot0p;
                if (temp1 > temp0)
                {
                    float numerator = temp1 - temp0;
                    float denominator = dot00 - 2.0f * dot01 + dot11;
                    if (numerator >= denominator) barycentricW = 1.0f;
                    else barycentricW = numerator / denominator;
                    barycentricV = 1.0f - barycentricW;
                }
                else
                {
                    barycentricW = 0.0f;
                    if (temp1 >= 0.0f) barycentricV = 0.0f;
                    else if (-temp1 >= dot00) barycentricV = 1.0f;
                    else barycentricV = -temp1 / dot00;
                }
            }
            else
            {
                // Region 1
                float numerator = (dot11 + dot1p) - (dot01 + dot0p);
                float denominator = dot00 - 2.0f * dot01 + dot11;
                if (numerator <= 0.0f) barycentricV = 0.0f;
                else if (numerator >= denominator) barycentricV = 1.0f;
                else barycentricV = numerator / denominator;
                barycentricW = 1.0f - barycentricV;
            }
        }

        return Vector3Add(triangle.v0, Vector3Add(Vector3Scale(edge0, barycentricV), Vector3Scale(edge1, barycentricW)));
    }

    bool Collision::IntersectSpherevsTriangle(const Vector3& sphereCenter, 
                                              float sphereRadius, 
                                              const CollisionTriangle& triangle, 
                                              Vector3& outMinimumTranslationVector)
    {
        Vector3 closestPoint = GetClosestPointOnTriangle(sphereCenter, triangle);
        Vector3 directionToSphere = Vector3Subtract(sphereCenter, closestPoint);
        float distanceSquared = Vector3LengthSqr(directionToSphere);

        if (distanceSquared > sphereRadius * sphereRadius) return false;

        float distance = sqrtf(distanceSquared);
        Vector3 collisionNormal = (distance > 0.0001f) ? Vector3Scale(directionToSphere, 1.0f / distance) : Vector3{0.0f, 1.0f, 0.0f};
        
        float penetrationDepth = sphereRadius - distance;
        outMinimumTranslationVector = Vector3Scale(collisionNormal, penetrationDepth);
        
        return true;
    }
}

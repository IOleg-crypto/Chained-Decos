#ifndef CH_FRUSTUM_H
#define CH_FRUSTUM_H

#include <raylib.h>
#include <raymath.h>
#include <array>
#include <cmath>

namespace CHEngine
{
struct Frustum
{
    static constexpr size_t PlaneCount = 6;

    enum PlaneIndex : size_t { Left = 0, Right, Bottom, Top, Near, Far };

    // Planes: x, y, z = нормаль, w = відстань до початку координат
    std::array<Vector4, PlaneCount> Planes;

    /**
     * Витягує площини з матриці View-Projection.
     * Raylib Matrix: m0, m4, m8, m12 - це перший рядок (Row-major storage).
     */
    void Extract(Matrix mat)
    {
        // Розрахунок площин (Gribb-Hartmann)
        Planes[Left]   = { mat.m3 + mat.m0, mat.m7 + mat.m4, mat.m11 + mat.m8, mat.m15 + mat.m12 };
        Planes[Right]  = { mat.m3 - mat.m0, mat.m7 - mat.m4, mat.m11 - mat.m8, mat.m15 - mat.m12 };
        Planes[Bottom] = { mat.m3 + mat.m1, mat.m7 + mat.m5, mat.m11 + mat.m9, mat.m15 + mat.m13 };
        Planes[Top]    = { mat.m3 - mat.m1, mat.m7 - mat.m5, mat.m11 - mat.m9, mat.m15 - mat.m13 };
        Planes[Near]   = { mat.m3 + mat.m2, mat.m7 + mat.m6, mat.m11 + mat.m10, mat.m15 + mat.m14 };
        Planes[Far]    = { mat.m3 - mat.m2, mat.m7 - mat.m6, mat.m11 - mat.m10, mat.m15 - mat.m14 };

        // Нормалізація для коректної роботи IsSphereVisible
        for (auto& p : Planes)
        {
            float length = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
            float invL = (length != 0.0f) ? 1.0f / length : 1.0f;
            p.x *= invL; p.y *= invL; p.z *= invL; p.w *= invL;
        }
    }

    /**
     * Найшвидша перевірка AABB через центр та екстенти.
     */
    bool IsBoxVisible(Vector3 center, Vector3 extents) const
    {
        for (const auto& p : Planes)
        {
            // Проєкція екстентів на нормаль площини
            float r = extents.x * fabsf(p.x) + extents.y * fabsf(p.y) + extents.z * fabsf(p.z);
            // Відстань від центру боксу до площини
            float d = p.x * center.x + p.y * center.y + p.z * center.z + p.w;

            if (d < -r) return false;
        }
        return true;
    }

    /**
     * Оптимізована перевірка локального AABB з урахуванням трансформації.
     * Замість 8 викликів Vector3Transform, ми робимо лише один.
     */
    bool IsBoxVisible(BoundingBox box, Matrix transform) const
    {
        // 1. Знаходимо локальний центр та екстенти
        Vector3 localCenter = { (box.max.x + box.min.x) * 0.5f, (box.max.y + box.min.y) * 0.5f, (box.max.z + box.min.z) * 0.5f };
        Vector3 localExtents = { (box.max.x - box.min.x) * 0.5f, (box.max.y - box.min.y) * 0.5f, (box.max.z - box.min.z) * 0.5f };

        // 2. Трансформуємо центр у світовий простір
        Vector3 worldCenter = Vector3Transform(localCenter, transform);

        // 3. Розраховуємо нові екстенти у світовому просторі (враховуючи обертання та масштаб)
        // Використовуємо абсолютні значення матриці, щоб отримати максимальне охоплення AABB
        Vector3 worldExtents = {
            fabsf(transform.m0) * localExtents.x + fabsf(transform.m4) * localExtents.y + fabsf(transform.m8) * localExtents.z,
            fabsf(transform.m1) * localExtents.x + fabsf(transform.m5) * localExtents.y + fabsf(transform.m9) * localExtents.z,
            fabsf(transform.m2) * localExtents.x + fabsf(transform.m6) * localExtents.y + fabsf(transform.m10) * localExtents.z
        };

        return IsBoxVisible(worldCenter, worldExtents);
    }

    bool IsSphereVisible(Vector3 center, float radius) const
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
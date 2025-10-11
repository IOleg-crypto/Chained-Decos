#include "Matrix4x4.h"
#include "Vector3.h"

namespace Engine {
namespace Math {

Matrix4x4 Matrix4x4::Identity() {
    return Matrix4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
}

Matrix4x4::Matrix4x4()
    : Matrix4x4::Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) {}

Matrix4x4::Matrix4x4(float m00, float m01, float m02, float m03,
                     float m10, float m11, float m12, float m13,
                     float m20, float m21, float m22, float m23,
                     float m30, float m31, float m32, float m33)
    : m00(m00), m01(m01), m02(m02), m03(m03)
    , m10(m10), m11(m11), m12(m12), m13(m13)
    , m20(m20), m21(m21), m22(m22), m23(m23)
    , m30(m30), m31(m31), m32(m32), m33(m33) {}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    return Matrix4x4(
        m00 * other.m00 + m01 * other.m10 + m02 * other.m20 + m03 * other.m30,
        m00 * other.m01 + m01 * other.m11 + m02 * other.m21 + m03 * other.m31,
        m00 * other.m02 + m01 * other.m12 + m02 * other.m22 + m03 * other.m32,
        m00 * other.m03 + m01 * other.m13 + m02 * other.m23 + m03 * other.m33,

        m10 * other.m00 + m11 * other.m10 + m12 * other.m20 + m13 * other.m30,
        m10 * other.m01 + m11 * other.m11 + m12 * other.m21 + m13 * other.m31,
        m10 * other.m02 + m11 * other.m12 + m12 * other.m22 + m13 * other.m32,
        m10 * other.m03 + m11 * other.m13 + m12 * other.m23 + m13 * other.m33,

        m20 * other.m00 + m21 * other.m10 + m22 * other.m20 + m23 * other.m30,
        m20 * other.m01 + m21 * other.m11 + m22 * other.m21 + m23 * other.m31,
        m20 * other.m02 + m21 * other.m12 + m22 * other.m22 + m23 * other.m32,
        m20 * other.m03 + m21 * other.m13 + m22 * other.m23 + m23 * other.m33,

        m30 * other.m00 + m31 * other.m10 + m32 * other.m20 + m33 * other.m30,
        m30 * other.m01 + m31 * other.m11 + m32 * other.m21 + m33 * other.m31,
        m30 * other.m02 + m31 * other.m12 + m32 * other.m22 + m33 * other.m32,
        m30 * other.m03 + m31 * other.m13 + m32 * other.m23 + m33 * other.m33
    );
}

Vector3 Matrix4x4::Transform(const Vector3& vector) const {
    return Vector3(
        m00 * vector.x + m01 * vector.y + m02 * vector.z + m03,
        m10 * vector.x + m11 * vector.y + m12 * vector.z + m13,
        m20 * vector.x + m21 * vector.y + m22 * vector.z + m23
    );
}

} // namespace Math
} // namespace Engine
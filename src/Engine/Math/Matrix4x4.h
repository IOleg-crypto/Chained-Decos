#ifndef MATRIX4X4_H
#define MATRIX4X4_H

namespace Engine {
namespace Math {

class Matrix4x4 {
public:
    // Identity matrix
    static Matrix4x4 Identity();

    // Default constructor (identity matrix)
    Matrix4x4();

    // Constructor with values
    Matrix4x4(float m00, float m01, float m02, float m03,
              float m10, float m11, float m12, float m13,
              float m20, float m21, float m22, float m23,
              float m30, float m31, float m32, float m33);

    // Matrix multiplication
    Matrix4x4 operator*(const Matrix4x4& other) const;

    // Vector transformation (assuming Vector3 is 3D)
    Vector3 Transform(const Vector3& vector) const;

    // Accessors
    float m00, m01, m02, m03;
    float m10, m11, m12, m13;
    float m20, m21, m22, m23;
    float m30, m31, m32, m33;
};

} // namespace Math
} // namespace Engine

#endif // MATRIX4X4_H
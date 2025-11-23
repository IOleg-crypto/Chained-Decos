#include "core/math/Vector/Vector3.h"
#include <cmath>

namespace Engine {
namespace Math {

float Vector3::Length() const {
    return std::sqrt(x * x + y * y + z * z);
}

float Vector3::LengthSquared() const {
    return x * x + y * y + z * z;
}

Vector3 Vector3::Normalized() const {
    float len = Length();
    if (len > 0.0f) {
        return *this / len;
    }
    return *this;
}

void Vector3::Normalize() {
    float len = Length();
    if (len > 0.0f) {
        x /= len;
        y /= len;
        z /= len;
    }
}

float Vector3::Dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vector3 Vector3::Cross(const Vector3& other) const {
    return Vector3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

}} // namespace Engine::Math

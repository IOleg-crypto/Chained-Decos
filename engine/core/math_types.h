#ifndef CH_MATH_TYPES_H
#define CH_MATH_TYPES_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{
    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector4 = glm::vec4;
    using Quaternion = glm::quat;
    using Matrix = glm::mat4;

    struct Color
    {
        unsigned char r, g, b, a;

        Color() : r(0), g(0), b(0), a(255) {}
        Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
            : r(r), g(g), b(b), a(a) {}
        
        static Color White() { return { 255, 255, 255, 255 }; }
        static Color Black() { return { 0, 0, 0, 255 }; }
        static Color Red()   { return { 255, 0, 0, 255 }; }
        static Color Green() { return { 0, 255, 0, 255 }; }
        static Color Blue()  { return { 0, 0, 255, 255 }; }
    };

    struct BoundingBox
    {
        glm::vec3 Min;
        glm::vec3 Max;
    };

    struct Ray
    {
        glm::vec3 position;
        glm::vec3 direction;
    };
}

#endif // CH_MATH_TYPES_H

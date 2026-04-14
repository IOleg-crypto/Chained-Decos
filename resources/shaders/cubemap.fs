#version 450 core

in vec3 fragPosition;

uniform sampler2D equirectangularMap;

out vec4 finalColor;

const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2*PI) та 1/PI

vec2 SampleSphericalMap(vec3 v)
{
    // atan2 повертає значення від -PI до PI
    // asin повертає від -PI/2 до PI/2
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    
    // Перетворюємо в діапазон [0, 1]
    uv *= invAtan;
    uv += 0.5;
    
    // stb_image завантажує JPG/PNG з перевертанням по V (stbi_set_flip_vertically_on_load).
    // Компенсуємо це тут, щоб кубмапа генерувалась правильно.
    uv.y = 1.0 - uv.y;
    
    return uv;
}

void main()
{       
    vec3 direction = normalize(fragPosition);
    vec2 uv = SampleSphericalMap(direction);
    
    // ВАЖЛИВО: textureLod замість texture прибирає шов (диру) 
    // на межі розгортки через ігнорування mipmap-градієнтів.
    vec3 color = textureLod(equirectangularMap, uv, 0.0).rgb;
    
    finalColor = vec4(color, 1.0);
}
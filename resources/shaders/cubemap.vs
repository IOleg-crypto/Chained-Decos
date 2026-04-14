#version 450 core

layout (location = 0) in vec3 a_Position;

uniform mat4 projection;
uniform mat4 view;

out vec3 fragPosition;

void main()
{
    fragPosition = a_Position;
    
    // Видаляємо трансляцію (переміщення) з матриці вигляду, 
    // щоб скайбокс завжди "слідував" за камерою
    mat4 viewRotationOnly = mat4(mat3(view));
    
    vec4 pos = projection * viewRotationOnly * vec4(a_Position, 1.0);
    
    // Не використовуємо .xyww тут, бо ми малюємо в пустий FBO без depth buffer.
    // Інакше через кліппінг може відсікатися Z=1.0.
    gl_Position = pos;
}
#version 450 core

layout (location = 0) in vec3 vertexPosition;

uniform mat4 projection;
uniform mat4 view;

out vec3 fragPosition;

void main()
{
    fragPosition = vertexPosition;
    
    // Видаляємо трансляцію (переміщення) з матриці вигляду, 
    // щоб скайбокс завжди "слідував" за камерою
    mat4 viewRotationOnly = mat4(mat3(view));
    
    vec4 pos = projection * viewRotationOnly * vec4(vertexPosition, 1.0);
    
    // Використовуємо .xyww, щоб після ділення на W глибина (Z) завжди була 1.0.
    // Це змушує скайбокс малюватися "за" всіма іншими об'єктами.
    gl_Position = pos.xyww;
}
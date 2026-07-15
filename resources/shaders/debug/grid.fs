#version 420 core

in vec3 v_WorldPos;

uniform vec3 u_CameraPos;
uniform vec4 u_GridColor;   // rgb = color, a = base alpha
uniform float u_GridSize;   // primary spacing (наприклад, 1.0)
uniform float u_FadeStart;  // distance where fade begins
uniform float u_FadeEnd;    // distance where grid is fully invisible

out vec4 o_Color;

// Функція для генерації антиаліасингової сітки товщиною в 1 піксель
float grid(vec2 coord, float spacing)
{
    vec2 grid_coord = coord / spacing;
    
    // Знаходимо похідні (швидкість зміни координат на піксель екрана)
    vec2 derivative = fwidth(grid_coord);
    
    // fract() створює повторюваний паттерн від 0 до 1
    vec2 grid_val = abs(fract(grid_coord - 0.5) - 0.5) / derivative;
    
    // Визначаємо мінімальне значення між X та Y компонентами
    float line_weight = min(grid_val.x, grid_val.y);
    
    // Повертаємо згладжену лінію товщиною в 1 піксель
    return 1.0 - min(line_weight, 1.0);
}

void main()
{
    // Отримуємо горизонтальні координати площини сітки (X, Z)
    vec2 coord = v_WorldPos.xz;

    // Первинна сітка (дрібна)
    float g1 = grid(coord, u_GridSize);
    // Вторинна сітка (велика, кожна 10-та лінія)
    float g2 = grid(coord, u_GridSize * 10.0);

    // Розрахунок згасання (Fade) залежно від відстані до камери
    float dist = length(v_WorldPos - u_CameraPos);
    
    // Запобігаємо діленню на нуль, якщо FadeStart == FadeEnd
    float fade_range = max(u_FadeEnd - u_FadeStart, 0.001);
    float fade = 1.0 - clamp((dist - u_FadeStart) / fade_range, 0.0, 1.0);
    fade = fade * fade; // Квадратичне згасання для плавності

    // Правильне змішування: 
    // Кожна 10-та лінія (g2) має бути яскравішою (наприклад, 0.9), ніж звичайні лінії (g1 * 0.3)
    float alpha_mask = max(g1 * 0.3, g2 * 0.9);
    float final_alpha = alpha_mask * fade;

    // Відсікаємо повністю прозорі пікселі, щоб зекономити ресурси (early discard)
    if (final_alpha < 0.005) discard;

    o_Color = vec4(u_GridColor.rgb, final_alpha * u_GridColor.a);
}
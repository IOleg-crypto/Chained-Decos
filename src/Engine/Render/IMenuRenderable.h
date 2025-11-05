#ifndef IMENURENDERABLE_H
#define IMENURENDERABLE_H

// Інтерфейс для UI елементів (Menu, HUD, тощо)
// Дотримується Interface Segregation Principle - тільки методи, потрібні для UI
struct IMenuRenderable
{
    virtual ~IMenuRenderable() = default;

    // Оновлення UI стану
    virtual void Update() = 0;
    
    // Рендеринг UI
    virtual void Render() = 0;
};

#endif // IMENURENDERABLE_H

#ifndef IMENURENDERABLE_H
#define IMENURENDERABLE_H

// Interface for UI elements (Menu, HUD, etc.)
// Follows Interface Segregation Principle - only methods needed for UI
struct IMenuRenderable
{
    virtual ~IMenuRenderable() = default;

    // Update UI state
    virtual void Update() = 0;
    
    // Render UI
    virtual void Render() = 0;
};

#endif // IMENURENDERABLE_H

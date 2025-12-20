#ifndef IGUI_MANAGER_H
#define IGUI_MANAGER_H

#include <memory>
#include <vector>

namespace ChainedDecos
{
class GuiElement;
}

class IGuiManager
{
public:
    virtual ~IGuiManager() = default;

    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;

    virtual void AddElement(std::shared_ptr<ChainedDecos::GuiElement> element) = 0;
    virtual void RemoveElement(std::shared_ptr<ChainedDecos::GuiElement> element) = 0;
    virtual void Clear() = 0;

    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;
};

#endif // IGUI_MANAGER_H

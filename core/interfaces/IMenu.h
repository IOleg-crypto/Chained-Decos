#ifndef IMENU_H
#define IMENU_H

#include <string>

// Minimal Menu interface
// Essential API only - 10 methods (down from 40+)
// Engine depends on this interface, not concrete Menu class.
class IMenu
{
public:
    virtual ~IMenu() = default;

    // Lifecycle
    virtual void Update() = 0;
    virtual void Render() = 0;

    // State
    virtual bool IsOpen() const = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;

    // Game flow
    virtual bool ShouldStartGame() const = 0;
    virtual std::string GetSelectedMapName() const = 0;
    virtual void SetGameInProgress(bool inProgress) = 0;
    virtual void ResetAction() = 0;

    // Console
    virtual void ToggleConsole() = 0;
    virtual bool IsConsoleOpen() const = 0;
};

#endif // IMENU_H





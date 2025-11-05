//
// Created by Kilo Code
//

#ifndef IUIMANAGER_H
#define IUIMANAGER_H

// UI Manager interface
class IUIManager {
public:
    virtual ~IUIManager() = default;
    virtual void Render() = 0;
    virtual void HandleInput() = 0;
    virtual void ShowObjectPanel(bool show) = 0;
    virtual void ShowPropertiesPanel(bool show) = 0;
    virtual void ShowParkourMapDialog(bool show) = 0;
    virtual int GetGridSize() const = 0;
};

#endif // IUIMANAGER_H
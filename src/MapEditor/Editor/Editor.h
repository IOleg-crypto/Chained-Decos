//
// Created by I#Oleg.
//

#ifndef EDITOR_H
#define EDITOR_H

#include <imgui.h>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

#include <CameraController/CameraController.h>

// Structure for map objects (cubes, spheres, cylinders)
struct MapObject
{
    Vector3 position; // Object position in 3D space
    Vector3 scale;    // Object scale (size)
    Vector3 rotation; // Object rotation in radians
    Color color;      // Object color
    std::string name; // Object name for identification
    int type;         // Object type: 0=cube, 1=sphere, 2=cylinder
    bool selected;    // Selection state for UI
    Vector2 size;     // If it`s plane or other meshes
    // For ellipse
    float radiusV;
    float radiusH;

    MapObject()
        : position{0, 0, 0}, scale{1, 1, 1}, rotation{0, 0, 0}, color{WHITE}, name(""), type(0),
          selected(false), size{2, 2}, radiusH(1), radiusV(1)
    {
    }
};

// Main editor class for the map editor
class Editor
{
  private:
    std::shared_ptr<CameraController> m_cameraController; // Camera controller for 3D view
    std::vector<MapObject> m_objects;                     // List of all objects in the scene
    int m_selectedObjectIndex;                            // Index of currently selected object
    int m_currentTool;                                    // Current editing tool
    bool m_showImGui;                                     // Show/hide ImGui interface
    bool m_showObjectPanel;                               // Show/hide object list panel
    bool m_showPropertiesPanel;                           // Show/hide properties panel
    bool m_shouldAddObject;                               // Add cube , cylinder or sphere

    // Available editing tools
    enum Tool
    {
        SELECT = 0,      // Select objects
        MOVE = 1,        // Move objects
        ROTATE = 2,      // Rotate objects
        SCALE = 3,       // Scale objects
        ADD_CUBE = 4,    // Add cube primitive
        ADD_SPHERE = 5,  // Add sphere primitive
        ADD_CYLINDER = 6 // Add cylinder primitive
    };

  public:
    Editor();
    ~Editor();

  public:
    // Core editor functions
    [[nodiscard]] std::shared_ptr<CameraController> GetCameraController() const;
    void Update();       // Update editor state
    void Render() const; // Render 3D objects
    void RenderImGui();  // Render ImGui interface
    void HandleInput();  // Handle user input

    // Object management functions
    void AddObject(const MapObject &obj); // Add new object to scene
    void RemoveObject(int index);         // Remove object by index
    void SelectObject(int index);         // Select object by index
    void ClearSelection();                // Clear current selection

    // File operations
    void SaveMap(const std::string &filename) const; // Save map to file
    void LoadMap(const std::string &filename);       // Load map from file

  private:
    // Rendering functions
    static void RenderObject(const MapObject &obj); // Render single object
    void RenderImGuiObjectPanel();                  // Render object list panel
    void RenderImGuiPropertiesPanel();              // Render properties panel
    void RenderImGuiToolbar();                      // Render toolbar

    // Input handling
    void PickObject();          // Handle mouse input
    void HandleKeyboardInput(); // Handle keyboard input
};

#endif // EDITOR_H

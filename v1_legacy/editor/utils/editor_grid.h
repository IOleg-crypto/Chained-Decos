#ifndef CD_EDITOR_UTILS_EDITOR_GRID_H
#define CD_EDITOR_UTILS_EDITOR_GRID_H

#include <raylib.h>

class EditorGrid
{
public:
    EditorGrid();
    ~EditorGrid();

    void Init();
    void Draw(Camera3D camera, int width, int height);

private:
    Shader m_Shader;
    int m_ViewLoc;
    int m_ProjLoc;
    int m_NearLoc;
    int m_FarLoc;
};

#endif // CD_EDITOR_UTILS_EDITOR_GRID_H

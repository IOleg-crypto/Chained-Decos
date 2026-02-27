#ifndef CH_MESH_DRAW_PARAMS_H
#define CH_MESH_DRAW_PARAMS_H

#include "raylib.h"
#include <vector>

namespace CHEngine
{
struct MeshDrawParams
{
    Matrix Transform = MatrixIdentity();
    Color Tint = WHITE;
    bool Wireframe = false;
    bool CastShadows = true;
    bool ReceiveShadows = true;
    
    // For skinned meshes
    const std::vector<Matrix>* BoneMatrices = nullptr;
    
    // Level Of Detail
    int LOD = 0;
};
}

#endif

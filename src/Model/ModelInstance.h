//
// Created by I#Oleg
//

#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include <raylib.h>

struct ModelInstance {
     Vector3 position; // Takes position(x , y , z)
     Model *pModel; //  Take model
     // Takes position and model
     ModelInstance(Vector3 pos , Model *pMdl);
};



#endif //MODELINSTANCE_H

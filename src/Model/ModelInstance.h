//
// Created by I#Oleg
//

#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include <raylib.h>
#include <string>

struct ModelInstance {
     Vector3 position; // Takes position(x , y , z)
     Model *pModel; //  Take model
     float scale; // Scale model;
     std::string modelName; // Name of model
     // Takes position and model
     ModelInstance(Vector3 pos , Model *pMdl , float scl , const std::string &name);
};



#endif //MODELINSTANCE_H

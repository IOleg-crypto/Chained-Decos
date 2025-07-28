//
// Created by I#Oleg
//

#include "ModelInstance.h"

// Default init
ModelInstance::ModelInstance(const Vector3 pos , Model *pMdl , const float scl , const std::string &name)
: position(pos) , pModel(pMdl) , scale(scl) , modelName(name) {
    // Nothing
}


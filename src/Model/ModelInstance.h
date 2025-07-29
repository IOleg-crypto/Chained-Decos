//
// Created by I#Oleg
//

#ifndef MODELINSTANCE_H
#define MODELINSTANCE_H

#include <raylib.h>
#include <string>

class ModelInstance {
private:
     Vector3 position; // Takes position(x , y , z)
     Model *pModel; //  Take model
     float scale; // Scale model;
     std::string modelName; // Name of model
     Color color ; // Color of model;
public:
     ModelInstance(Vector3 pos , Model *pMdl , float scl , const std::string &name , Color color);
     ModelInstance(Vector3 pos , Model *pMdl , float scl , const std::string &name);
public:
     // Take properties of model
     [[nodiscard]] std::string getModelName() const;
     [[nodiscard]] Color getColor() const;
     [[nodiscard]] float getScale() const;
     [[nodiscard]] Model *getModel() const;
     [[nodiscard]] Vector3 getModelPosition() const;
};



#endif //MODELINSTANCE_H

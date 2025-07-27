//
// Created by I#Oleg
//

#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <raylib.h>

class Models {
private:
    std::vector<Model> m_models;
public:
    Models() = default;
    ~Models();
public:
    void AddModel(const std::string& modelPath);
    void DrawAll(float x , float y , float z) const; // Draw all models
    Model& GetModel(size_t index); // Access to one model;
};



#endif //MODEL_H

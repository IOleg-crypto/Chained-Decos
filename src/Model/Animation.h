#ifndef ANIMATION_H
#define ANIMATION_H

#include <Model/Model.h>
#include <raylib.h>
#include <string>

class Animation
{
public:
    Animation();
    ~Animation();

public:
    void Update(Model &model);
    void LoadAnimations(const std::string &path);

private:
    int m_animCount;
    unsigned int m_animIndex;
    unsigned int m_animCurrentFrame;
    ModelAnimation *m_modelAnimations;
};

#endif /* ANIMATION_H */

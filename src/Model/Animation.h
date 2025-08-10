//
//  Created by I#Oleg
//
#ifndef ANIMATION_H
#define ANIMATION_H

#include <raylib.h>
#include <string>

class Animation
{
public:
    Animation();
    ~Animation();

public:
    void Update(Model &model);
    bool LoadAnimations(const std::string &path);
    void SetAnimationIndex(unsigned int index);

private:
    int m_animCount;
    unsigned int m_animIndex;
    unsigned int m_animCurrentFrame;
    ModelAnimation *m_modelAnimations;
};

#endif /* ANIMATION_H */
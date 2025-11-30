#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include <raylib.h>
#include <string>
#include <unordered_map>


struct AudioComponent
{
    // Sound library для цього entity
    std::unordered_map<std::string, std::string> soundPaths; // name -> path

    // 3D audio settings
    bool is3D = true;
    float volume = 1.0f;
    float maxDistance = 100.0f;
    float minDistance = 1.0f;

    // Currently playing
    std::string currentSound;
    bool isPlaying = false;
    bool loop = false;
};

#endif // AUDIO_COMPONENT_H

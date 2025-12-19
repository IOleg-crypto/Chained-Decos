#ifndef IAUDIO_MANAGER_H
#define IAUDIO_MANAGER_H

#include <string>

class IAudioManager
{
public:
    virtual ~IAudioManager() = default;
    
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void SetMasterVolume(float volume) = 0;
};

#endif // IAUDIO_MANAGER_H





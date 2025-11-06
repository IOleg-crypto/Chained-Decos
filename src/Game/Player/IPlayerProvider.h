#ifndef IPLAYER_PROVIDER_H
#define IPLAYER_PROVIDER_H

// Forward declaration
class Player;

// Professional interface for accessing Player
// Allows getting Player without dependency on Kernel
class IPlayerProvider
{
public:
    virtual ~IPlayerProvider() = default;
    virtual Player* GetPlayer() = 0;
};

#endif // IPLAYER_PROVIDER_H


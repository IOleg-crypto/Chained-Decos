#ifndef IMAP_MANAGER_PROVIDER_H
#define IMAP_MANAGER_PROVIDER_H


class MapManager;

class IMapManagerProvider
{
public:
    virtual ~IMapManagerProvider() = default;
    virtual MapManager* GetMapManager() = 0;
};

#endif // IMAP_MANAGER_PROVIDER_H


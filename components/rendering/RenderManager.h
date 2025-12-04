#ifndef SERVERS_RENDER_MANAGER_H
#define SERVERS_RENDER_MANAGER_H

#include <raylib.h>
#include <string>

namespace Servers
{

class RenderManager
{
public:
    RenderManager() = default;
    ~RenderManager();

    // Non-copyable
    RenderManager(const RenderManager &) = delete;
    RenderManager &operator=(const RenderManager &) = delete;

    // Lifecycle
    bool Initialize(int width, int height, const std::string &title);
    void Shutdown();

    // Frame control
    void BeginFrame();
    void EndFrame();

    // 3D rendering
    void BeginMode3D(Camera3D camera);
    void EndMode3D();

    // Accessors
    int GetScreenWidth() const;
    int GetScreenHeight() const;

    // Debug
    void SetDebug(bool enabled);
    bool IsDebug() const;

private:
    int m_width = 1280;
    int m_height = 720;
    bool m_debug = false;
};

} // namespace Servers

#endif // SERVERS_RENDER_MANAGER_H

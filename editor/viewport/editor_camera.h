#ifndef CH_EDITOR_CAMERA_H
#define CH_EDITOR_CAMERA_H

#include <raylib.h>

namespace CHEngine
{

class EditorCamera
{
public:
    EditorCamera();
    ~EditorCamera() = default;

    void OnUpdate(float deltaTime);

    const Camera3D &GetRaylibCamera() const;
    Camera3D &GetRaylibCamera();

    void SetPosition(Vector3 pos);
    void SetTarget(Vector3 target);
    void SetFOV(float fov);

private:
    Camera3D m_Camera;
};

} // namespace CHEngine

#endif // CH_EDITOR_CAMERA_H

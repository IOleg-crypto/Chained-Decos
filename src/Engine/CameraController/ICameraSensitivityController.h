#ifndef ICAMERA_SENSITIVITY_CONTROLLER_H
#define ICAMERA_SENSITIVITY_CONTROLLER_H

// Professional interface for camera sensitivity control
// Allows Menu to work with any camera implementation
class ICameraSensitivityController
{
public:
    virtual ~ICameraSensitivityController() = default;
    
    // Set mouse sensitivity
    virtual void SetMouseSensitivity(float sensitivity) = 0;
    
    // Get current sensitivity
    virtual float GetMouseSensitivity() const = 0;
};

#endif // ICAMERA_SENSITIVITY_CONTROLLER_H


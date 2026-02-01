#ifndef CH_UI_MATH_H
#define CH_UI_MATH_H

#include <glm/glm.hpp>
#include "components/control_component.h"

namespace CHEngine::UIMath
{
using vec2 = glm::vec2;

// Screen-space rectangle (absolute pixel coordinates)
struct Rect
{
    vec2 Min;  // Top-left
    vec2 Max;  // Bottom-right
    
    vec2 Size() const { return Max - Min; }
    vec2 Center() const { return (Min + Max) * 0.5f; }
    
    bool Contains(vec2 point) const 
    {
        return point.x >= Min.x && point.x <= Max.x &&
               point.y >= Min.y && point.y <= Max.y;
    }
};

/**
 * Simplified RectTransform calculation.
 * Anchors are 0..1 relative to viewport size.
 * Offsets are absolute pixels.
 */
inline Rect CalculateRect(const RectTransform& transform, 
                          vec2 viewportSize,
                          vec2 viewportOffset = {0.0f, 0.0f})
{
    // 1. Calculate the box defined by anchors (clamped to 0..1)
    vec2 clAnchMin = glm::clamp(transform.AnchorMin, vec2(0.0f), vec2(1.0f));
    vec2 clAnchMax = glm::clamp(transform.AnchorMax, vec2(0.0f), vec2(1.0f));

    vec2 anchorMinPos = {
        viewportSize.x * clAnchMin.x,
        viewportSize.y * clAnchMin.y
    };
    vec2 anchorMaxPos = {
        viewportSize.x * clAnchMax.x,
        viewportSize.y * clAnchMax.y
    };
    
    // 2. Add offsets (absolute pixels)
    // pMin: top-left corner
    // pMax: bottom-right corner
    vec2 pMin = anchorMinPos + transform.OffsetMin;
    vec2 pMax = anchorMaxPos + transform.OffsetMax;
    
    // 3. Apply Pivot (only if AnchorMin == AnchorMax, for fixed-size elements)
    if (transform.AnchorMin == transform.AnchorMax)
    {
        vec2 size = pMax - pMin;
        vec2 pivotShift = {
            size.x * transform.Pivot.x,
            size.y * transform.Pivot.y
        };
        // This is a common way to use pivot with fixed size:
        // OffsetMin/Max are defined relative to the pivot point.
        // But for simplicity, we'll assume OffsetMin/Max are the final box 
        // if users aren't explicitly asking for pivot-based centering yet.
        // Given the scene data, users define OffsetMin/Max as absolute pixel bounds.
    }

    // Convert to screen coordinates (adding viewport offset)
    return Rect{
        {viewportOffset.x + pMin.x, viewportOffset.y + pMin.y},
        {viewportOffset.x + pMax.x, viewportOffset.y + pMax.y}
    };
}

} // namespace CHEngine::UIMath

#endif // CH_UI_MATH_H

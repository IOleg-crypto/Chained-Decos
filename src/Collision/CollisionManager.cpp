#include <Collision/CollisionManager.h>
#include <algorithm>
#include <raylib.h>
#include <vector>

void CollisionManager::AddCollider(Collision &collider) { 
    m_collisions.emplace_back(collider); 
}

void CollisionManager::ClearColliders() { m_collisions.clear(); }

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    return std::any_of(m_collisions.begin(), m_collisions.end(),
                       [&](const Collision &collider)
                       {
                           // Use hybrid collision system (automatically chooses optimal method)
                           return playerCollision.Intersects(collider);
                       });
}

bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response) const
{
    // Early exit if no colliders are registered
    if (m_collisions.size() == 0) {
        TraceLog(LOG_WARNING, "⚠️ NO COLLIDERS FOUND! Total colliders: %zu", m_collisions.size());
        return false;
    }

    // Get player AABB for collision calculations
    Vector3 playerMin = playerCollision.GetMin();
    Vector3 playerMax = playerCollision.GetMax();

    // Check collision against each registered collider
    int colliderIndex = 0;
    for (const auto &collider : m_collisions) {
        Vector3 colliderMin = collider.GetMin();
        Vector3 colliderMax = collider.GetMax();

        // Use hybrid collision system (automatically chooses optimal method based on complexity)
        bool hasCollision = playerCollision.Intersects(collider);
        
        // Log complex collisions for debugging (skip simple AABB to reduce spam)
        // Only log every 10th collision to reduce spam
        static int collisionLogCounter = 0;
        if (hasCollision && collider.GetCollisionType() != CollisionType::AABB_ONLY && (collisionLogCounter++ % 10 == 0)) {
            CollisionType playerType = playerCollision.GetCollisionType();
            CollisionType colliderType = collider.GetCollisionType();
            TraceLog(LOG_INFO, "🔍 Complex collision detected - Player type: %d, Collider type: %d", 
                     (int)playerType, (int)colliderType);
        }

        colliderIndex++;

        if (hasCollision) {
            // TraceLog(LOG_INFO,
            //          "🎯 COLLISION with [%d]! Player(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f) vs Collider(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)", 
            //          colliderIndex - 1, 
            //          playerMin.x, playerMin.y, playerMin.z, playerMax.x, playerMax.y, playerMax.z,
            //          colliderMin.x, colliderMin.y, colliderMin.z, colliderMax.x, colliderMax.y, colliderMax.z);

            // Identify collision type for appropriate handling
            bool isGroundPlane = (collider.GetCollisionType() == CollisionType::AABB_ONLY);
            bool isComplexModel = (collider.GetCollisionType() == CollisionType::OCTREE_ONLY || 
                                  collider.GetCollisionType() == CollisionType::TRIANGLE_PRECISE ||
                                  collider.GetCollisionType() == CollisionType::IMPROVED_AABB);
            
            // Log complex model collisions for debugging (reduced frequency)
            static int complexModelLogCounter = 0;
            if (isComplexModel && (complexModelLogCounter++ % 20 == 0)) {
                TraceLog(LOG_INFO, "🏗️ Complex model collision detected - using AABB-based MTV for stability");
            }

            // Calculate Minimum Translation Vector (MTV) for collision resolution
            // MTV is the smallest vector needed to separate two overlapping objects
            Vector3 aMin = playerCollision.GetMin();  // Player minimum corner
            Vector3 aMax = playerCollision.GetMax();  // Player maximum corner
            Vector3 bMin = collider.GetMin();         // Collider minimum corner
            Vector3 bMax = collider.GetMax();         // Collider maximum corner

            // Calculate overlap distances in both directions for each axis
            float dx1 = bMax.x - aMin.x;  // Distance to push player right
            float dx2 = aMax.x - bMin.x;  // Distance to push player left
            float dy1 = bMax.y - aMin.y;  // Distance to push player up
            float dy2 = aMax.y - bMin.y;  // Distance to push player down
            float dz1 = bMax.z - aMin.z;  // Distance to push player forward
            float dz2 = aMax.z - bMin.z;  // Distance to push player backward

            // Choose the smaller distance for each axis (minimum separation)
            float dx = (dx1 < dx2) ? dx1 : -dx2;
            float dy = (dy1 < dy2) ? dy1 : -dy2;
            float dz = (dz1 < dz2) ? dz1 : -dz2;
            
            // Get absolute values for comparison
            float absDx = fabsf(dx);
            float absDy = fabsf(dy);
            float absDz = fabsf(dz);

            // TraceLog(LOG_INFO, "📏 MTV distances: dx=%.3f, dy=%.3f, dz=%.3f (abs: %.3f, %.3f, %.3f)", 
            //          dx, dy, dz, absDx, absDy, absDz);

            // Anti-jittering: Ignore very small collisions but be more precise
            const float COLLISION_THRESHOLD = 0.1f;  // Much smaller threshold for precision
            float minDistance = (absDx < absDy && absDx < absDz) ? absDx : 
                               (absDy < absDz) ? absDy : absDz;
                               
            if (minDistance < COLLISION_THRESHOLD) {
                // Collision is too small - might be jittering, ignore it
                continue;
            }
            
            // Only clamp extremely large responses for complex models (not ground plane)
            if (isComplexModel) {
                const float MAX_COMPLEX_RESPONSE = 5.0f; // Reasonable limit to prevent huge pushes
                
                if (absDx > MAX_COMPLEX_RESPONSE) {
                    dx = (dx > 0) ? MAX_COMPLEX_RESPONSE : -MAX_COMPLEX_RESPONSE;
                    TraceLog(LOG_WARNING, "⚠️ Clamped complex model X-axis response from %.2f to %.2f", absDx, MAX_COMPLEX_RESPONSE);
                }
                if (absDy > MAX_COMPLEX_RESPONSE) {
                    dy = (dy > 0) ? MAX_COMPLEX_RESPONSE : -MAX_COMPLEX_RESPONSE;
                    TraceLog(LOG_WARNING, "⚠️ Clamped complex model Y-axis response from %.2f to %.2f", absDy, MAX_COMPLEX_RESPONSE);
                }
                if (absDz > MAX_COMPLEX_RESPONSE) {
                    dz = (dz > 0) ? MAX_COMPLEX_RESPONSE : -MAX_COMPLEX_RESPONSE;
                    TraceLog(LOG_WARNING, "⚠️ Clamped complex model Z-axis response from %.2f to %.2f", absDz, MAX_COMPLEX_RESPONSE);
                }
            }
            // Ground plane responses are NOT clamped - let them work naturally
            
            // Recalculate absolute values after clamping
            absDx = fabsf(dx);
            absDy = fabsf(dy);
            absDz = fabsf(dz);

            // Improved collision response prioritizing floor detection
            // Check if this is primarily a floor/ceiling collision
            const float FLOOR_DETECTION_RATIO = 0.8f; // Y must be at least 80% of the dominant axis
            
            // For complex models (walls/objects), be more strict about what counts as "floor"
            // Only treat as floor if it's clearly a vertical collision AND it's upward movement
            bool isFloorCollision = false;
            if (isGroundPlane) {
                // Ground plane is always treated as floor if Y is dominant
                isFloorCollision = (absDy > 0.001f && (absDy >= absDx * FLOOR_DETECTION_RATIO && absDy >= absDz * FLOOR_DETECTION_RATIO));
            } else if (isComplexModel) {
                // For complex models, only treat as floor if:
                // 1. Y is clearly dominant (stronger ratio)
                // 2. Response is upward (dy > 0)
                // 3. Player is falling (not jumping up)
                const float COMPLEX_FLOOR_RATIO = 1.5f; // Stricter ratio for complex models
                isFloorCollision = (dy > 0.001f && absDy > absDx * COMPLEX_FLOOR_RATIO && absDy > absDz * COMPLEX_FLOOR_RATIO);
            }
            
            if (isFloorCollision) {
                // This is primarily a vertical collision (floor/ceiling)
                response = {0, dy, 0};
            }
            else if (absDx >= absDy && absDx >= absDz) {
                // X-axis collision (wall) - don't interfere with falling
                response = {dx, 0, 0};
            }
            else if (absDz >= absDy && absDz >= absDx) {
                // Z-axis collision (wall) - don't interfere with falling
                response = {0, 0, dz};
            }
            else {
                // Fallback - only use Y if it's clearly a floor collision
                if (isGroundPlane || dy > 0) {
                    response = {0, dy, 0};
                } else {
                    // For complex models with unclear collision, prefer horizontal response
                    response = (absDx > absDz) ? Vector3{dx, 0, 0} : Vector3{0, 0, dz};
                }
            }

            float responseLength = Vector3Length(response);
            
            // Check for extreme collision on any axis (player deeply stuck inside collider)
            float maxAxisResponse = fmaxf(fmaxf(absDx, absDy), absDz);
            float extremeThreshold;
            if (isGroundPlane) {
                // Ground plane should rarely be considered "extreme" - only for truly massive responses
                extremeThreshold = 10.0f; // Reduced from 100.0f - even ground shouldn't push more than 10 units
            } else if (isComplexModel) {
                // Complex models - check individual axes instead of vector length
                extremeThreshold = 8.0f; // Reduced from 50.0f
            } else {
                extremeThreshold = 5.0f; // Reduced from 25.0f
            }
            
            // Check for extreme collision on any single axis (not vector length)
            if (maxAxisResponse > extremeThreshold) {
                TraceLog(LOG_ERROR, "🚨 EXTREME COLLISION: Player stuck deep inside %s collider (max axis: %.2f) - setting stuck marker", 
                         isGroundPlane ? "ground" : isComplexModel ? "complex model" : "unknown", maxAxisResponse);
                // Return a special "stuck" response that Player can detect
                response = Vector3Scale(Vector3Normalize(response), 999.0f); // Special marker
                TraceLog(LOG_INFO, "🔧 Stuck marker set: response length now %.2f", Vector3Length(response));
                return true;
            }
            
            // Only limit response for complex models, let ground plane work naturally
            if (isComplexModel) {
                // Limit each axis separately instead of vector length to prevent direction changes
                const float MAX_X_RESPONSE = 1.5f; // Reduced to prevent large pushes
                const float MAX_Y_RESPONSE = 2.0f; // Allow more Y movement for floor detection
                const float MAX_Z_RESPONSE = 1.5f; // Reduced to prevent large pushes
                
                bool clamped = false;
                if (fabsf(response.x) > MAX_X_RESPONSE) {
                    response.x = (response.x > 0) ? MAX_X_RESPONSE : -MAX_X_RESPONSE;
                    clamped = true;
                }
                if (fabsf(response.y) > MAX_Y_RESPONSE) {
                    response.y = (response.y > 0) ? MAX_Y_RESPONSE : -MAX_Y_RESPONSE;
                    clamped = true;
                }
                if (fabsf(response.z) > MAX_Z_RESPONSE) {
                    response.z = (response.z > 0) ? MAX_Z_RESPONSE : -MAX_Z_RESPONSE;
                    clamped = true;
                }
                
                if (clamped) {
                    TraceLog(LOG_WARNING, "⚠️ Complex model collision response clamped per-axis: (%.2f,%.2f,%.2f)", 
                             response.x, response.y, response.z);
                }
            }
            // Ground plane responses are NOT limited - let them work naturally

            // TraceLog(LOG_INFO, "📐 Final response vector: (%.3f, %.3f, %.3f) length=%.3f", 
            //          response.x, response.y, response.z, Vector3Length(response));

            return true;
        }
    }
    return false;
}
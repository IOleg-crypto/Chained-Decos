#ifndef PARTICLEEDITOR_H
#define PARTICLEEDITOR_H

#include <raylib.h>
#include <vector>
#include <string>
#include <memory>

#include "MapObject.h"

// Particle emitter types
enum class EmitterType
{
    POINT = 0,
    BOX = 1,
    SPHERE = 2,
    CIRCLE = 3,
    CONE = 4,
    MESH = 5
};

// Particle types
enum class ParticleType
{
    SPRITE = 0,
    MODEL = 1,
    TRAIL = 2,
    LIGHT = 3
};

// Particle properties
struct ParticleProperties
{
    // Basic properties
    std::string name;
    ParticleType type;
    bool enabled;

    // Emission properties
    EmitterType emitterType;
    Vector3 emitterPosition;
    Vector3 emitterSize;
    float emissionRate;     // Particles per second
    int maxParticles;
    float particleLifetime; // In seconds

    // Velocity properties
    Vector3 initialVelocity;
    Vector3 velocityVariation;
    float speed;
    float speedVariation;
    bool inheritVelocity;

    // Physics properties
    Vector3 gravity;
    float damping;
    float airResistance;
    bool collideWithTerrain;
    float bounce;
    float friction;

    // Size properties
    float startSize;
    float endSize;
    Vector3 sizeVariation;

    // Color properties
    Color startColor;
    Color endColor;
    float colorVariation;

    // Rotation properties
    float startRotation;
    float endRotation;
    float rotationSpeed;
    Vector3 rotationAxis;

    // Texture/Model properties
    std::string texturePath;
    std::string modelPath;
    Texture2D texture;
    Model model;
    bool resourcesLoaded;

    // Animation properties
    bool animated;
    int frameCount;
    float animationSpeed;
    bool loopAnimation;

    // Lighting properties (for light particles)
    Color lightColor;
    float lightIntensity;
    float lightRange;

    ParticleProperties() :
        name("New Particle System"),
        type(ParticleType::SPRITE),
        enabled(true),
        emitterType(EmitterType::POINT),
        emitterPosition({0.0f, 0.0f, 0.0f}),
        emitterSize({1.0f, 1.0f, 1.0f}),
        emissionRate(10.0f),
        maxParticles(100),
        particleLifetime(5.0f),
        initialVelocity({0.0f, 1.0f, 0.0f}),
        velocityVariation({0.0f, 0.0f, 0.0f}),
        speed(1.0f),
        speedVariation(0.0f),
        inheritVelocity(false),
        gravity({0.0f, -9.81f, 0.0f}),
        damping(0.99f),
        airResistance(0.0f),
        collideWithTerrain(false),
        bounce(0.5f),
        friction(0.5f),
        startSize(0.1f),
        endSize(0.05f),
        sizeVariation({0.0f, 0.0f, 0.0f}),
        startColor(WHITE),
        endColor(WHITE),
        colorVariation(0.0f),
        startRotation(0.0f),
        endRotation(0.0f),
        rotationSpeed(0.0f),
        rotationAxis({0.0f, 0.0f, 1.0f}),
        resourcesLoaded(false),
        animated(false),
        frameCount(1),
        animationSpeed(1.0f),
        loopAnimation(true),
        lightColor(WHITE),
        lightIntensity(1.0f),
        lightRange(5.0f)
    {}
};

// Individual particle data
struct Particle
{
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    float life;           // Current life (0 to lifetime)
    float lifetime;       // Total lifetime
    float size;
    Color color;
    float rotation;
    float rotationSpeed;
    Vector3 initialPosition;
    int frame;           // Current animation frame
    bool active;

    Particle() : life(0.0f), lifetime(1.0f), size(1.0f), rotation(0.0f), rotationSpeed(0.0f), frame(0), active(false) {}
};

// Particle effects editor
class ParticleEditor
{
private:
    std::vector<ParticleProperties> m_particleSystems;
    std::vector<std::vector<Particle>> m_particles;
    int m_selectedSystem;

    // Preview system
    bool m_previewMode;
    float m_previewTime;
    Vector3 m_previewPosition;

    // Emission timing
    float m_lastEmissionTime;
    float m_accumulatedTime;

    // Performance settings
    int m_maxTotalParticles;
    bool m_limitParticles;

public:
    ParticleEditor();
    ~ParticleEditor();

    // Initialization
    bool Initialize();
    void Cleanup();

    // Particle system management
    int CreateParticleSystem(const std::string& name = "New Particle System");
    bool DeleteParticleSystem(int index);
    bool DuplicateParticleSystem(int index);
    bool RenameParticleSystem(int index, const std::string& newName);

    // Particle system properties
    void SetSystemType(int systemIndex, ParticleType type);
    void SetEmitterType(int systemIndex, EmitterType type);
    void SetEmissionRate(int systemIndex, float rate);
    void SetMaxParticles(int systemIndex, int maxParticles);
    void SetParticleLifetime(int systemIndex, float lifetime);
    void SetGravity(int systemIndex, const Vector3& gravity);
    void SetStartColor(int systemIndex, const Color& color);
    void SetEndColor(int systemIndex, const Color& color);

    // Resource management
    bool LoadTexture(int systemIndex, const std::string& texturePath);
    bool LoadModel(int systemIndex, const std::string& modelPath);
    void UnloadResources(int systemIndex);

    // Selection
    void SelectSystem(int index);
    int GetSelectedSystem() const { return m_selectedSystem; }
    ParticleProperties* GetSelectedSystemProperties();

    // Playback control
    void PlaySystem(int systemIndex);
    void StopSystem(int systemIndex);
    void PauseSystem(int systemIndex);
    void RestartSystem(int systemIndex);

    // Preview
    void StartPreview(const Vector3& position);
    void StopPreview();
    void UpdatePreview(float deltaTime);

    // Emission control
    void EmitParticles(int systemIndex, float deltaTime);
    void UpdateParticles(int systemIndex, float deltaTime);
    void KillParticle(int systemIndex, int particleIndex);
    void KillAllParticles(int systemIndex);

    // Utility functions
    int GetParticleCount(int systemIndex) const;
    int GetTotalParticleCount() const;
    Vector3 GetEmitterPosition(int systemIndex) const;
    bool IsSystemPlaying(int systemIndex) const;

    // File operations
    bool SaveParticleSystem(int index, const std::string& filePath);
    bool LoadParticleSystem(const std::string& filePath);
    bool ExportParticleSystem(int index, const std::string& filePath);

    // Rendering
    void Render();
    void RenderParticleSystem(int systemIndex);
    void RenderEmitterGizmo(int systemIndex);
    void RenderPreview();

    // Serialization
    std::string SerializeSystem(int index) const;
    bool DeserializeSystem(const std::string& data, int index);

private:
    // Helper functions
    void InitializeParticle(Particle& particle, const ParticleProperties& properties);
    Vector3 GenerateEmitterPosition(const ParticleProperties& properties);
    Vector3 GenerateInitialVelocity(const ParticleProperties& properties);
    Color GenerateParticleColor(const ParticleProperties& properties, float lifeRatio);
    float GenerateParticleSize(const ParticleProperties& properties, float lifeRatio);

    void UpdateParticlePhysics(Particle& particle, const ParticleProperties& properties, float deltaTime);
    void UpdateParticleAnimation(Particle& particle, const ParticleProperties& properties, float deltaTime);

    bool LoadTextureInternal(ParticleProperties& properties, const std::string& texturePath);
    bool LoadModelInternal(ParticleProperties& properties, const std::string& modelPath);

    // Emitter shape functions
    Vector3 GetRandomPointInPointEmitter();
    Vector3 GetRandomPointInBoxEmitter(const Vector3& size);
    Vector3 GetRandomPointInSphereEmitter(float radius);
    Vector3 GetRandomPointInCircleEmitter(float radius);
    Vector3 GetRandomPointInConeEmitter(const Vector3& direction, float angle, float length);
};

#endif // PARTICLEEDITOR_H
#include "ParticleEditor.h"
#include <raylib.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>

ParticleEditor::ParticleEditor()
    : m_selectedSystem(-1)
    , m_previewMode(false)
    , m_previewTime(0.0f)
    , m_lastEmissionTime(0.0f)
    , m_accumulatedTime(0.0f)
    , m_maxTotalParticles(10000)
    , m_limitParticles(true)
{
}

ParticleEditor::~ParticleEditor()
{
    Cleanup();
}

bool ParticleEditor::Initialize()
{
    // Initialize random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    return true;
}

void ParticleEditor::Cleanup()
{
    // Stop all systems and clean up resources
    for (size_t i = 0; i < m_particleSystems.size(); ++i)
    {
        UnloadResources(static_cast<int>(i));
    }

    m_particleSystems.clear();
    m_particles.clear();
    m_selectedSystem = -1;
}

int ParticleEditor::CreateParticleSystem(const std::string& name)
{
    ParticleProperties system;
    system.name = name.empty() ? "ParticleSystem_" + std::to_string(m_particleSystems.size()) : name;

    m_particleSystems.push_back(system);
    m_particles.push_back(std::vector<Particle>());

    return static_cast<int>(m_particleSystems.size()) - 1;
}

bool ParticleEditor::DeleteParticleSystem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_particleSystems.size()))
        return false;

    // Unload resources first
    UnloadResources(index);

    // Remove system and particles
    m_particleSystems.erase(m_particleSystems.begin() + index);
    m_particles.erase(m_particles.begin() + index);

    if (m_selectedSystem >= static_cast<int>(m_particleSystems.size()))
    {
        m_selectedSystem = static_cast<int>(m_particleSystems.size()) - 1;
    }

    return true;
}

bool ParticleEditor::DuplicateParticleSystem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_particleSystems.size()))
        return false;

    ParticleProperties duplicated = m_particleSystems[index];
    duplicated.name = duplicated.name + "_Copy";

    m_particleSystems.push_back(duplicated);
    m_particles.push_back(std::vector<Particle>());

    return true;
}

bool ParticleEditor::RenameParticleSystem(int index, const std::string& newName)
{
    if (index < 0 || index >= static_cast<int>(m_particleSystems.size()) || newName.empty())
        return false;

    m_particleSystems[index].name = newName;
    return true;
}

void ParticleEditor::SetSystemType(int systemIndex, ParticleType type)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].type = type;
    }
}

void ParticleEditor::SetEmitterType(int systemIndex, EmitterType type)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].emitterType = type;
    }
}

void ParticleEditor::SetEmissionRate(int systemIndex, float rate)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].emissionRate = std::max(0.0f, rate);
    }
}

void ParticleEditor::SetMaxParticles(int systemIndex, int maxParticles)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].maxParticles = std::max(1, maxParticles);

        // Resize particle vector if necessary
        if (static_cast<int>(m_particles[systemIndex].size()) > maxParticles)
        {
            m_particles[systemIndex].resize(maxParticles);
        }
    }
}

void ParticleEditor::SetParticleLifetime(int systemIndex, float lifetime)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].particleLifetime = std::max(0.1f, lifetime);
    }
}

void ParticleEditor::SetGravity(int systemIndex, const Vector3& gravity)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].gravity = gravity;
    }
}

void ParticleEditor::SetStartColor(int systemIndex, const Color& color)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].startColor = color;
    }
}

void ParticleEditor::SetEndColor(int systemIndex, const Color& color)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].endColor = color;
    }
}

bool ParticleEditor::LoadTexture(int systemIndex, const std::string& texturePath)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return false;

    return LoadTextureInternal(m_particleSystems[systemIndex], texturePath);
}

bool ParticleEditor::LoadModel(int systemIndex, const std::string& modelPath)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return false;

    return LoadModelInternal(m_particleSystems[systemIndex], modelPath);
}

void ParticleEditor::UnloadResources(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return;

    ParticleProperties& properties = m_particleSystems[systemIndex];

    if (properties.resourcesLoaded)
    {
        if (properties.type == ParticleType::SPRITE && properties.texture.id != 0)
        {
            UnloadTexture(properties.texture);
        }
        else if (properties.type == ParticleType::MODEL && properties.model.meshCount > 0)
        {
            UnloadModel(properties.model);
        }

        properties.resourcesLoaded = false;
    }
}

void ParticleEditor::SelectSystem(int index)
{
    if (index >= -1 && index < static_cast<int>(m_particleSystems.size()))
    {
        m_selectedSystem = index;
    }
}

ParticleProperties* ParticleEditor::GetSelectedSystemProperties()
{
    if (m_selectedSystem >= 0 && m_selectedSystem < static_cast<int>(m_particleSystems.size()))
    {
        return &m_particleSystems[m_selectedSystem];
    }
    return nullptr;
}

void ParticleEditor::PlaySystem(int systemIndex)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].enabled = true;
    }
}

void ParticleEditor::StopSystem(int systemIndex)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].enabled = false;
        KillAllParticles(systemIndex);
    }
}

void ParticleEditor::PauseSystem(int systemIndex)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        m_particleSystems[systemIndex].enabled = false;
    }
}

void ParticleEditor::RestartSystem(int systemIndex)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        KillAllParticles(systemIndex);
        m_particleSystems[systemIndex].enabled = true;
        m_lastEmissionTime = 0.0f;
        m_accumulatedTime = 0.0f;
    }
}

void ParticleEditor::StartPreview(const Vector3& position)
{
    m_previewMode = true;
    m_previewPosition = position;
    m_previewTime = 0.0f;

    if (m_selectedSystem >= 0)
    {
        // Set preview system position
        m_particleSystems[m_selectedSystem].emitterPosition = position;
    }
}

void ParticleEditor::StopPreview()
{
    m_previewMode = false;
    if (m_selectedSystem >= 0)
    {
        KillAllParticles(m_selectedSystem);
    }
}

void ParticleEditor::UpdatePreview(float deltaTime)
{
    if (!m_previewMode || m_selectedSystem < 0)
        return;

    m_previewTime += deltaTime;

    // Update particles for preview
    UpdateParticles(m_selectedSystem, deltaTime);
    EmitParticles(m_selectedSystem, deltaTime);
}

void ParticleEditor::EmitParticles(int systemIndex, float deltaTime)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return;

    ParticleProperties& properties = m_particleSystems[systemIndex];

    if (!properties.enabled)
        return;

    // Check particle limit
    if (m_limitParticles && GetTotalParticleCount() >= m_maxTotalParticles)
        return;

    // Calculate how many particles to emit this frame
    m_accumulatedTime += deltaTime;
    float emissionPeriod = 1.0f / properties.emissionRate;

    while (m_accumulatedTime >= emissionPeriod && properties.enabled)
    {
        // Find an available particle slot
        std::vector<Particle>& particles = m_particles[systemIndex];

        int availableSlot = -1;
        for (int i = 0; i < static_cast<int>(particles.size()); ++i)
        {
            if (!particles[i].active)
            {
                availableSlot = i;
                break;
            }
        }

        // If no slot available and under max particles, add new particle
        if (availableSlot == -1 && static_cast<int>(particles.size()) < properties.maxParticles)
        {
            particles.push_back(Particle());
            availableSlot = static_cast<int>(particles.size()) - 1;
        }

        // If still no slot, skip emission
        if (availableSlot == -1)
            break;

        // Initialize the particle
        Particle& particle = particles[availableSlot];
        InitializeParticle(particle, properties);

        m_accumulatedTime -= emissionPeriod;
    }
}

void ParticleEditor::UpdateParticles(int systemIndex, float deltaTime)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return;

    ParticleProperties& properties = m_particleSystems[systemIndex];
    std::vector<Particle>& particles = m_particles[systemIndex];

    for (int i = static_cast<int>(particles.size()) - 1; i >= 0; --i)
    {
        Particle& particle = particles[i];

        if (!particle.active)
            continue;

        // Update particle life
        particle.life += deltaTime;

        if (particle.life >= particle.lifetime)
        {
            particle.active = false;
            continue;
        }

        // Update particle physics
        UpdateParticlePhysics(particle, properties, deltaTime);

        // Update particle animation
        UpdateParticleAnimation(particle, properties, deltaTime);
    }
}

void ParticleEditor::KillParticle(int systemIndex, int particleIndex)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particles.size()) &&
        particleIndex >= 0 && particleIndex < static_cast<int>(m_particles[systemIndex].size()))
    {
        m_particles[systemIndex][particleIndex].active = false;
    }
}

void ParticleEditor::KillAllParticles(int systemIndex)
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particles.size()))
    {
        for (auto& particle : m_particles[systemIndex])
        {
            particle.active = false;
        }
    }
}

int ParticleEditor::GetParticleCount(int systemIndex) const
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particles.size()))
        return 0;

    int count = 0;
    for (const auto& particle : m_particles[systemIndex])
    {
        if (particle.active)
            count++;
    }
    return count;
}

int ParticleEditor::GetTotalParticleCount() const
{
    int total = 0;
    for (const auto& particles : m_particles)
    {
        for (const auto& particle : particles)
        {
            if (particle.active)
                total++;
        }
    }
    return total;
}

Vector3 ParticleEditor::GetEmitterPosition(int systemIndex) const
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        return m_particleSystems[systemIndex].emitterPosition;
    }
    return {0.0f, 0.0f, 0.0f};
}

bool ParticleEditor::IsSystemPlaying(int systemIndex) const
{
    if (systemIndex >= 0 && systemIndex < static_cast<int>(m_particleSystems.size()))
    {
        return m_particleSystems[systemIndex].enabled;
    }
    return false;
}

bool ParticleEditor::SaveParticleSystem(int index, const std::string& filePath)
{
    if (index < 0 || index >= static_cast<int>(m_particleSystems.size()))
        return false;

    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to save particle system: " << filePath << std::endl;
        return false;
    }

    file << SerializeSystem(index);
    return true;
}

bool ParticleEditor::LoadParticleSystem(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to load particle system: " << filePath << std::endl;
        return false;
    }

    // Read file content and parse
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    int newIndex = CreateParticleSystem("Loaded System");
    return DeserializeSystem(content, newIndex);
}

bool ParticleEditor::ExportParticleSystem(int index, const std::string& filePath)
{
    // Export for use in game engine
    return SaveParticleSystem(index, filePath);
}

void ParticleEditor::Render()
{
    // Render all active particle systems
    for (size_t i = 0; i < m_particleSystems.size(); ++i)
    {
        if (m_particleSystems[i].enabled)
        {
            RenderParticleSystem(static_cast<int>(i));
            RenderEmitterGizmo(static_cast<int>(i));
        }
    }

    if (m_previewMode)
    {
        RenderPreview();
    }
}

void ParticleEditor::RenderParticleSystem(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return;

    ParticleProperties& properties = m_particleSystems[systemIndex];
    std::vector<Particle>& particles = m_particles[systemIndex];

    for (const auto& particle : particles)
    {
        if (!particle.active)
            continue;

        float lifeRatio = particle.life / particle.lifetime;
        Color particleColor = GenerateParticleColor(properties, lifeRatio);
        float particleSize = GenerateParticleSize(properties, lifeRatio);

        switch (properties.type)
        {
            case ParticleType::SPRITE:
                if (properties.resourcesLoaded)
                {
                    // Draw sprite particle
                    DrawBillboard(GetCamera3D(), properties.texture, particle.position,
                                particleSize, particleColor);
                }
                else
                {
                    // Draw as point
                    DrawSphere(particle.position, particleSize * 0.1f, particleColor);
                }
                break;

            case ParticleType::MODEL:
                if (properties.resourcesLoaded)
                {
                    // Draw model particle
                    Matrix transform = MatrixMultiply(
                        MatrixScale(particleSize, particleSize, particleSize),
                        MatrixMultiply(
                            MatrixRotateAxis(particle.rotationAxis, particle.rotation * DEG2RAD),
                            MatrixTranslate(particle.position.x, particle.position.y, particle.position.z)
                        )
                    );
                    DrawModel(properties.model, particle.position, particleSize, particleColor);
                }
                break;

            case ParticleType::TRAIL:
                // Draw trail effect
                if (particle.life > 0.1f)
                {
                    Vector3 trailEnd = Vector3Add(particle.position, Vector3Scale(particle.velocity, -0.1f));
                    DrawLine3D(particle.position, trailEnd, Fade(particleColor, 0.5f));
                }
                break;

            case ParticleType::LIGHT:
                // Light particles don't render visually, they affect lighting
                break;
        }
    }
}

void ParticleEditor::RenderEmitterGizmo(int systemIndex)
{
    if (systemIndex < 0 || systemIndex >= static_cast<int>(m_particleSystems.size()))
        return;

    ParticleProperties& properties = m_particleSystems[systemIndex];

    // Draw emitter shape based on type
    switch (properties.emitterType)
    {
        case EmitterType::POINT:
            DrawSphere(properties.emitterPosition, 0.1f, SKYBLUE);
            break;

        case EmitterType::BOX:
            DrawCubeV(properties.emitterPosition, properties.emitterSize, SKYBLUE);
            DrawCubeWiresV(properties.emitterPosition, properties.emitterSize, BLUE);
            break;

        case EmitterType::SPHERE:
            DrawSphere(properties.emitterPosition, properties.emitterSize.x, SKYBLUE);
            DrawSphereWires(properties.emitterPosition, properties.emitterSize.x, 16, 16, BLUE);
            break;

        case EmitterType::CIRCLE:
            // Draw circle on XZ plane
            for (int i = 0; i < 32; ++i)
            {
                float angle1 = (i / 32.0f) * 2.0f * PI;
                float angle2 = ((i + 1) / 32.0f) * 2.0f * PI;
                Vector3 start = {
                    properties.emitterPosition.x + cosf(angle1) * properties.emitterSize.x,
                    properties.emitterPosition.y,
                    properties.emitterPosition.z + sinf(angle1) * properties.emitterSize.x
                };
                Vector3 end = {
                    properties.emitterPosition.x + cosf(angle2) * properties.emitterSize.x,
                    properties.emitterPosition.y,
                    properties.emitterPosition.z + sinf(angle2) * properties.emitterSize.x
                };
                DrawLine3D(start, end, BLUE);
            }
            break;

        case EmitterType::CONE:
            // Draw cone shape
            break;

        case EmitterType::MESH:
            // Draw mesh-based emitter
            break;
    }
}

void ParticleEditor::RenderPreview()
{
    // Render preview interface elements
}

std::string ParticleEditor::SerializeSystem(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_particleSystems.size()))
        return "";

    // Serialize particle system to string format
    return "ParticleSystem:" + m_particleSystems[index].name;
}

bool ParticleEditor::DeserializeSystem(const std::string& data, int index)
{
    if (index < 0 || index >= static_cast<int>(m_particleSystems.size()))
        return false;

    // Parse data and update system properties
    // For now, just return true as placeholder
    return true;
}

void ParticleEditor::InitializeParticle(Particle& particle, const ParticleProperties& properties)
{
    particle.active = true;
    particle.life = 0.0f;
    particle.lifetime = properties.particleLifetime;
    particle.position = Vector3Add(properties.emitterPosition, GenerateEmitterPosition(properties));
    particle.initialPosition = particle.position;
    particle.velocity = GenerateInitialVelocity(properties);
    particle.acceleration = {0.0f, 0.0f, 0.0f};
    particle.size = properties.startSize;
    particle.color = properties.startColor;
    particle.rotation = properties.startRotation;
    particle.rotationSpeed = properties.rotationSpeed;
    particle.frame = 0;
}

Vector3 ParticleEditor::GenerateEmitterPosition(const ParticleProperties& properties)
{
    switch (properties.emitterType)
    {
        case EmitterType::POINT:
            return GetRandomPointInPointEmitter();
        case EmitterType::BOX:
            return GetRandomPointInBoxEmitter(properties.emitterSize);
        case EmitterType::SPHERE:
            return GetRandomPointInSphereEmitter(properties.emitterSize.x);
        case EmitterType::CIRCLE:
            return GetRandomPointInCircleEmitter(properties.emitterSize.x);
        case EmitterType::CONE:
            return GetRandomPointInConeEmitter({0.0f, 1.0f, 0.0f}, 45.0f, properties.emitterSize.x);
        default:
            return {0.0f, 0.0f, 0.0f};
    }
}

Vector3 ParticleEditor::GenerateInitialVelocity(const ParticleProperties& properties)
{
    Vector3 baseVelocity = properties.initialVelocity;

    // Add variation
    Vector3 variation = {
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * properties.velocityVariation.x,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * properties.velocityVariation.y,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * properties.velocityVariation.z
    };

    return Vector3Add(baseVelocity, variation);
}

Color ParticleEditor::GenerateParticleColor(const ParticleProperties& properties, float lifeRatio)
{
    // Interpolate between start and end colors
    unsigned char r = static_cast<unsigned char>(properties.startColor.r * (1.0f - lifeRatio) + properties.endColor.r * lifeRatio);
    unsigned char g = static_cast<unsigned char>(properties.startColor.g * (1.0f - lifeRatio) + properties.endColor.g * lifeRatio);
    unsigned char b = static_cast<unsigned char>(properties.startColor.b * (1.0f - lifeRatio) + properties.endColor.b * lifeRatio);
    unsigned char a = static_cast<unsigned char>(properties.startColor.a * (1.0f - lifeRatio) + properties.endColor.a * lifeRatio);

    return {r, g, b, a};
}

float ParticleEditor::GenerateParticleSize(const ParticleProperties& properties, float lifeRatio)
{
    return properties.startSize * (1.0f - lifeRatio) + properties.endSize * lifeRatio;
}

void ParticleEditor::UpdateParticlePhysics(Particle& particle, const ParticleProperties& properties, float deltaTime)
{
    // Apply gravity
    particle.acceleration = properties.gravity;

    // Apply air resistance
    if (properties.airResistance > 0.0f)
    {
        particle.acceleration = Vector3Add(particle.acceleration,
                                         Vector3Scale(particle.velocity, -properties.airResistance));
    }

    // Update velocity and position
    particle.velocity = Vector3Add(particle.velocity, Vector3Scale(particle.acceleration, deltaTime));
    particle.velocity = Vector3Scale(particle.velocity, properties.damping);
    particle.position = Vector3Add(particle.position, Vector3Scale(particle.velocity, deltaTime));

    // Handle collisions with terrain if enabled
    if (properties.collideWithTerrain)
    {
        // Simple ground collision
        if (particle.position.y <= 0.0f)
        {
            particle.position.y = 0.0f;
            particle.velocity.y *= -properties.bounce;
            particle.velocity.x *= (1.0f - properties.friction);
            particle.velocity.z *= (1.0f - properties.friction);
        }
    }
}

void ParticleEditor::UpdateParticleAnimation(Particle& particle, const ParticleProperties& properties, float deltaTime)
{
    if (!properties.animated)
        return;

    // Update rotation
    particle.rotation += particle.rotationSpeed * deltaTime;

    // Update animation frame
    if (properties.frameCount > 1)
    {
        float frameTime = properties.particleLifetime / properties.frameCount;
        int currentFrame = static_cast<int>(particle.life / frameTime);
        particle.frame = std::min(currentFrame, properties.frameCount - 1);
    }
}

bool ParticleEditor::LoadTextureInternal(ParticleProperties& properties, const std::string& texturePath)
{
    if (properties.resourcesLoaded && properties.type == ParticleType::SPRITE)
    {
        UnloadTexture(properties.texture);
    }

    properties.texture = LoadTexture(texturePath.c_str());
    properties.resourcesLoaded = properties.texture.id != 0;
    properties.texturePath = texturePath;

    if (properties.resourcesLoaded)
    {
        std::cout << "Loaded particle texture: " << texturePath << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to load particle texture: " << texturePath << std::endl;
        return false;
    }
}

bool ParticleEditor::LoadModelInternal(ParticleProperties& properties, const std::string& modelPath)
{
    if (properties.resourcesLoaded && properties.type == ParticleType::MODEL)
    {
        UnloadModel(properties.model);
    }

    properties.model = LoadModel(modelPath.c_str());
    properties.resourcesLoaded = properties.model.meshCount > 0;
    properties.modelPath = modelPath;

    if (properties.resourcesLoaded)
    {
        std::cout << "Loaded particle model: " << modelPath << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to load particle model: " << modelPath << std::endl;
        return false;
    }
}

Vector3 ParticleEditor::GetRandomPointInPointEmitter()
{
    return {0.0f, 0.0f, 0.0f};
}

Vector3 ParticleEditor::GetRandomPointInBoxEmitter(const Vector3& size)
{
    return {
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * size.x,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * size.y,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * size.z
    };
}

Vector3 ParticleEditor::GetRandomPointInSphereEmitter(float radius)
{
    float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
    float phi = acosf(2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f);
    float r = radius * cbrtf(static_cast<float>(rand()) / RAND_MAX);

    return {
        r * sinf(phi) * cosf(theta),
        r * sinf(phi) * sinf(theta),
        r * cosf(phi)
    };
}

Vector3 ParticleEditor::GetRandomPointInCircleEmitter(float radius)
{
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
    float r = radius * sqrtf(static_cast<float>(rand()) / RAND_MAX);

    return {
        r * cosf(angle),
        0.0f,
        r * sinf(angle)
    };
}

Vector3 ParticleEditor::GetRandomPointInConeEmitter(const Vector3& direction, float angle, float length)
{
    float coneAngle = angle * DEG2RAD;
    float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
    float phi = static_cast<float>(rand()) / RAND_MAX * coneAngle;

    Vector3 randomDir = {
        sinf(phi) * cosf(theta),
        cosf(phi),
        sinf(phi) * sinf(theta)
    };

    return Vector3Scale(randomDir, length * static_cast<float>(rand()) / RAND_MAX);
}
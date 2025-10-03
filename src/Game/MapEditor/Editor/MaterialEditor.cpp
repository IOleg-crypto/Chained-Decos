#include "MaterialEditor.h"
#include <raylib.h>
#include <algorithm>
#include <iostream>
#include <fstream>

MaterialEditor::MaterialEditor()
    : m_selectedMaterial(-1)
    , m_selectedTexture(-1)
    , m_previewGenerated(false)
    , m_showTexturePanel(true)
    , m_showMaterialPanel(true)
    , m_showPreviewPanel(true)
{
}

MaterialEditor::~MaterialEditor()
{
    Cleanup();
}

bool MaterialEditor::Initialize()
{
    // Set up default materials
    SetupDefaultMaterials();

    // Generate preview mesh
    GeneratePreviewMesh();

    return true;
}

void MaterialEditor::Cleanup()
{
    if (m_previewGenerated)
    {
        UnloadModel(m_materialPreviewSphere);
        m_previewGenerated = false;
    }

    // Clean up loaded textures
    for (auto& pair : m_loadedTextures)
    {
        UnloadTexture(pair.second);
    }
    m_loadedTextures.clear();

    for (auto& texture : m_textureLibrary)
    {
        if (texture.loaded)
        {
            UnloadTexture(texture.texture);
        }
    }
    m_textureLibrary.clear();

    m_materials.clear();
    m_selectedMaterial = -1;
    m_selectedTexture = -1;
}

int MaterialEditor::CreateMaterial(const std::string& name)
{
    MaterialProperties material;
    material.name = name.empty() ? "Material " + std::to_string(m_materials.size()) : name;
    m_materials.push_back(material);
    return static_cast<int>(m_materials.size()) - 1;
}

bool MaterialEditor::DeleteMaterial(int index)
{
    if (index < 0 || index >= static_cast<int>(m_materials.size()))
        return false;

    m_materials.erase(m_materials.begin() + index);

    if (m_selectedMaterial >= static_cast<int>(m_materials.size()))
    {
        m_selectedMaterial = static_cast<int>(m_materials.size()) - 1;
    }

    return true;
}

bool MaterialEditor::DuplicateMaterial(int index)
{
    if (index < 0 || index >= static_cast<int>(m_materials.size()))
        return false;

    MaterialProperties duplicated = m_materials[index];
    duplicated.name = duplicated.name + " Copy";
    m_materials.push_back(duplicated);

    return true;
}

bool MaterialEditor::RenameMaterial(int index, const std::string& newName)
{
    if (index < 0 || index >= static_cast<int>(m_materials.size()) || newName.empty())
        return false;

    m_materials[index].name = newName;
    return true;
}

void MaterialEditor::SetMaterialType(int materialIndex, MaterialType type)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_materials[materialIndex].type = type;
        UpdateMaterialShader(materialIndex);
    }
}

void MaterialEditor::SetMaterialBaseColor(int materialIndex, const Color& color)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_materials[materialIndex].baseColor = color;
    }
}

void MaterialEditor::SetMaterialMetallic(int materialIndex, float metallic)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_materials[materialIndex].metallic = std::max(0.0f, std::min(1.0f, metallic));
    }
}

void MaterialEditor::SetMaterialRoughness(int materialIndex, float roughness)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_materials[materialIndex].roughness = std::max(0.0f, std::min(1.0f, roughness));
    }
}

void MaterialEditor::SetMaterialEmissive(int materialIndex, float intensity)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_materials[materialIndex].emissiveIntensity = std::max(0.0f, intensity);
    }
}

void MaterialEditor::SetMaterialTransparency(int materialIndex, float transparency)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_materials[materialIndex].transparency = std::max(0.0f, std::min(1.0f, transparency));
    }
}

bool MaterialEditor::LoadTexture(const std::string& filePath)
{
    MaterialTexture texture;
    if (LoadTextureInternal(filePath, texture))
    {
        m_textureLibrary.push_back(texture);
        return true;
    }
    return false;
}

bool MaterialEditor::RemoveTexture(int index)
{
    if (index < 0 || index >= static_cast<int>(m_textureLibrary.size()))
        return false;

    UnloadTextureInternal(m_textureLibrary[index]);
    m_textureLibrary.erase(m_textureLibrary.begin() + index);

    if (m_selectedTexture >= static_cast<int>(m_textureLibrary.size()))
    {
        m_selectedTexture = static_cast<int>(m_textureLibrary.size()) - 1;
    }

    return true;
}

void MaterialEditor::ClearTextureLibrary()
{
    for (auto& texture : m_textureLibrary)
    {
        if (texture.loaded)
        {
            UnloadTexture(texture.texture);
        }
    }
    m_textureLibrary.clear();
    m_selectedTexture = -1;
}

bool MaterialEditor::AssignTextureToMaterial(int materialIndex, TextureType textureType, int textureIndex)
{
    if (materialIndex < 0 || materialIndex >= static_cast<int>(m_materials.size()) ||
        textureIndex < 0 || textureIndex >= static_cast<int>(m_textureLibrary.size()))
        return false;

    // This would assign the texture to the material for the specified texture type
    // For now, just return true as a placeholder
    return true;
}

bool MaterialEditor::RemoveTextureFromMaterial(int materialIndex, TextureType textureType)
{
    if (materialIndex < 0 || materialIndex >= static_cast<int>(m_materials.size()))
        return false;

    // This would remove the texture from the material for the specified texture type
    // For now, just return true as a placeholder
    return true;
}

void MaterialEditor::SelectMaterial(int index)
{
    if (index >= -1 && index < static_cast<int>(m_materials.size()))
    {
        m_selectedMaterial = index;
    }
}

void MaterialEditor::SelectTexture(int index)
{
    if (index >= -1 && index < static_cast<int>(m_textureLibrary.size()))
    {
        m_selectedTexture = index;
    }
}

MaterialProperties* MaterialEditor::GetMaterial(int index)
{
    if (index >= 0 && index < static_cast<int>(m_materials.size()))
    {
        return &m_materials[index];
    }
    return nullptr;
}

const MaterialProperties* MaterialEditor::GetMaterial(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_materials.size()))
    {
        return &m_materials[index];
    }
    return nullptr;
}

MaterialTexture* MaterialEditor::GetTexture(int index)
{
    if (index >= 0 && index < static_cast<int>(m_textureLibrary.size()))
    {
        return &m_textureLibrary[index];
    }
    return nullptr;
}

const MaterialTexture* MaterialEditor::GetTexture(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_textureLibrary.size()))
    {
        return &m_textureLibrary[index];
    }
    return nullptr;
}

void MaterialEditor::GenerateMaterialPreview(int materialIndex)
{
    if (materialIndex < 0 || materialIndex >= static_cast<int>(m_materials.size()))
        return;

    // Generate a preview sphere with the selected material
    // This would apply the material properties to the preview mesh
}

void MaterialEditor::RenderMaterialPreview(const Vector3& position, float scale)
{
    if (m_previewGenerated)
    {
        DrawModel(m_materialPreviewSphere, position, scale, WHITE);
    }
}

std::vector<std::string> MaterialEditor::GetSupportedTextureFormats() const
{
    return {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds"};
}

bool MaterialEditor::IsTextureFormatSupported(const std::string& extension) const
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const std::string& supported : GetSupportedTextureFormats())
    {
        if (ext == supported) return true;
    }
    return false;
}

std::string MaterialEditor::GetMaterialInfo(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_materials.size()))
        return "Invalid Material";

    const MaterialProperties& material = m_materials[index];
    return material.name + " (" + std::to_string(static_cast<int>(material.type)) + ")";
}

std::string MaterialEditor::GetTextureInfo(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_textureLibrary.size()))
        return "Invalid Texture";

    const MaterialTexture& texture = m_textureLibrary[index];
    return texture.filePath + " (" + std::to_string(texture.texture.width) + "x" +
           std::to_string(texture.texture.height) + ")";
}

bool MaterialEditor::SaveMaterialLibrary(const std::string& filePath)
{
    // Save material library to file
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to save material library: " << filePath << std::endl;
        return false;
    }

    // Write material data as JSON or custom format
    file << "MaterialLibrary\n";
    file << "Count: " << m_materials.size() << "\n";

    for (const auto& material : m_materials)
    {
        file << MaterialToJson(material) << "\n";
    }

    return true;
}

bool MaterialEditor::LoadMaterialLibrary(const std::string& filePath)
{
    // Load material library from file
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to load material library: " << filePath << std::endl;
        return false;
    }

    // Read and parse material data
    std::string line;
    if (std::getline(file, line) && line == "MaterialLibrary")
    {
        if (std::getline(file, line) && line.substr(0, 6) == "Count:")
        {
            size_t count = std::stoul(line.substr(7));
            m_materials.clear();

            for (size_t i = 0; i < count && std::getline(file, line); ++i)
            {
                MaterialProperties material = JsonToMaterial(line);
                m_materials.push_back(material);
            }
        }
    }

    return true;
}

bool MaterialEditor::ExportMaterial(int index, const std::string& filePath)
{
    if (index < 0 || index >= static_cast<int>(m_materials.size()))
        return false;

    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to export material: " << filePath << std::endl;
        return false;
    }

    file << MaterialToJson(m_materials[index]);
    return true;
}

void MaterialEditor::Render()
{
    // Main rendering function - would render material editor UI
}

void MaterialEditor::RenderMaterialPanel()
{
    // Render material selection and editing panel
}

void MaterialEditor::RenderTexturePanel()
{
    // Render texture library panel
}

void MaterialEditor::RenderPreviewPanel()
{
    // Render material preview panel
}

void MaterialEditor::ApplyMaterialToObject(MapObject& object, int materialIndex)
{
    if (materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        // Apply material properties to object
        // This would set material information in the MapObject
    }
}

MaterialProperties MaterialEditor::GetMaterialForObject(const MapObject& object) const
{
    // Get material for a specific object
    // For now, return a default material
    MaterialProperties defaultMaterial;
    return defaultMaterial;
}

void MaterialEditor::SetupDefaultMaterials()
{
    // Create some default materials

    // Default gray material
    MaterialProperties defaultMat;
    defaultMat.name = "Default";
    defaultMat.baseColor = LIGHTGRAY;
    m_materials.push_back(defaultMat);

    // Metal material
    MaterialProperties metalMat;
    metalMat.name = "Metal";
    metalMat.type = MaterialType::METALLIC;
    metalMat.baseColor = GRAY;
    metalMat.metallic = 1.0f;
    metalMat.roughness = 0.2f;
    m_materials.push_back(metalMat);

    // Glass material
    MaterialProperties glassMat;
    glassMat.name = "Glass";
    glassMat.type = MaterialType::GLASS;
    glassMat.baseColor = SKYBLUE;
    glassMat.transparency = 0.8f;
    glassMat.refractiveIndex = 1.5f;
    m_materials.push_back(glassMat);

    // Emissive material
    MaterialProperties emissiveMat;
    emissiveMat.name = "Emissive";
    emissiveMat.type = MaterialType::EMISSIVE;
    emissiveMat.baseColor = BLUE;
    emissiveMat.emissiveIntensity = 2.0f;
    m_materials.push_back(emissiveMat);

    m_selectedMaterial = 0;
}

void MaterialEditor::GeneratePreviewMesh()
{
    if (m_previewGenerated)
    {
        UnloadModel(m_materialPreviewSphere);
    }

    // Create a sphere for material preview
    m_materialPreviewSphere = LoadModelFromMesh(GenMeshSphere(1.0f, 16, 16));
    m_previewGenerated = true;
}

void MaterialEditor::UpdateMaterialShader(int materialIndex)
{
    if (materialIndex < 0 || materialIndex >= static_cast<int>(m_materials.size()))
        return;

    // Update shader based on material type
    const MaterialProperties& material = m_materials[materialIndex];

    // This would set up appropriate shaders for different material types
    switch (material.type)
    {
        case MaterialType::STANDARD:
            // Set up standard PBR shader
            break;
        case MaterialType::METALLIC:
            // Set up metallic shader
            break;
        case MaterialType::GLASS:
            // Set up glass/refraction shader
            break;
        case MaterialType::EMISSIVE:
            // Set up emissive shader
            break;
        case MaterialType::TRANSPARENT:
            // Set up transparent shader
            break;
    }
}

bool MaterialEditor::LoadTextureInternal(const std::string& filePath, MaterialTexture& texture)
{
    texture.texture = LoadTexture(filePath.c_str());
    texture.loaded = texture.texture.id != 0;
    texture.filePath = filePath;

    if (texture.loaded)
    {
        std::cout << "Loaded texture: " << filePath
                  << " (" << texture.texture.width << "x" << texture.texture.height << ")" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return false;
    }
}

void MaterialEditor::UnloadTextureInternal(MaterialTexture& texture)
{
    if (texture.loaded)
    {
        UnloadTexture(texture.texture);
        texture.loaded = false;
    }
}

bool MaterialEditor::ValidateMaterial(const MaterialProperties& material) const
{
    return !material.name.empty();
}

bool MaterialEditor::ValidateTexture(const MaterialTexture& texture) const
{
    return texture.loaded && !texture.filePath.empty();
}

std::string MaterialEditor::MaterialToJson(const MaterialProperties& material) const
{
    // Convert material to JSON string
    return "{\"name\":\"" + material.name + "\",\"type\":" + std::to_string(static_cast<int>(material.type)) + "}";
}

std::string MaterialEditor::TextureToJson(const MaterialTexture& texture) const
{
    // Convert texture to JSON string
    return "{\"path\":\"" + texture.filePath + "\",\"type\":" + std::to_string(static_cast<int>(texture.type)) + "}";
}

MaterialProperties MaterialEditor::JsonToMaterial(const std::string& json) const
{
    // Parse JSON string to material
    MaterialProperties material;
    // Simple parsing for now
    material.name = "Parsed Material";
    return material;
}

MaterialTexture MaterialEditor::JsonToTexture(const std::string& json) const
{
    // Parse JSON string to texture
    MaterialTexture texture;
    // Simple parsing for now
    return texture;
}
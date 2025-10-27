#ifndef MATERIALEDITOR_H
#define MATERIALEDITOR_H

#include <raylib.h>
#include <vector>
#include <string>
#include <memory>
#include <map>

#include "MapObject.h"

// Material types
enum class MaterialType
{
    STANDARD = 0,
    METALLIC = 1,
    GLASS = 2,
    EMISSIVE = 3,
    TRANSPARENT = 4
};

// Texture types for materials
enum class TextureType
{
    DIFFUSE = 0,
    NORMAL = 1,
    METALLIC = 2,
    ROUGHNESS = 3,
    EMISSIVE = 4,
    AMBIENT_OCCLUSION = 5,
    HEIGHT = 6
};

// Material texture information
struct MaterialTexture
{
    TextureType type;
    std::string filePath;
    Texture texture;
    bool loaded;
    Vector2 scale;
    Vector2 offset;
    float rotation;

    MaterialTexture() : loaded(false), scale({1.0f, 1.0f}), offset({0.0f, 0.0f}), rotation(0.0f) {}
};

// Material properties
struct MaterialProperties
{
    std::string name;
    MaterialType type;
    Color baseColor;
    float metallic;
    float roughness;
    float emissiveIntensity;
    float transparency;
    float refractiveIndex; // For glass materials
    bool doubleSided;

    // PBR properties
    float specular;
    float specularTint;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;

    MaterialProperties() :
        name("New Material"),
        type(MaterialType::STANDARD),
        baseColor(WHITE),
        metallic(0.0f),
        roughness(0.5f),
        emissiveIntensity(0.0f),
        transparency(0.0f),
        refractiveIndex(1.5f),
        doubleSided(false),
        specular(0.5f),
        specularTint(0.0f),
        sheen(0.0f),
        sheenTint(0.0f),
        clearcoat(0.0f),
        clearcoatGloss(0.0f)
    {}
};

// Material editor for managing materials and textures
class MaterialEditor
{
private:
    std::vector<MaterialProperties> m_materials;
    std::vector<MaterialTexture> m_textureLibrary;
    int m_selectedMaterial;
    int m_selectedTexture;

    // Material preview
    Model m_materialPreviewSphere;
    bool m_previewGenerated;

    // Texture management
    std::string m_textureDirectory;
    std::map<std::string, Texture2D> m_loadedTextures;

    // Material editing state
    bool m_showTexturePanel;
    bool m_showMaterialPanel;
    bool m_showPreviewPanel;

public:
    MaterialEditor();
    ~MaterialEditor();

    // Initialization
    bool Initialize();
    void Cleanup();

    // Material management
    int CreateMaterial(const std::string& name = "New Material");
    bool DeleteMaterial(int index);
    bool DuplicateMaterial(int index);
    bool RenameMaterial(int index, const std::string& newName);

    // Material properties
    void SetMaterialType(int materialIndex, MaterialType type);
    void SetMaterialBaseColor(int materialIndex, const Color& color);
    void SetMaterialMetallic(int materialIndex, float metallic);
    void SetMaterialRoughness(int materialIndex, float roughness);
    void SetMaterialEmissive(int materialIndex, float intensity);
    void SetMaterialTransparency(int materialIndex, float transparency);

    // Texture management
    bool LoadTexture(const std::string& filePath);
    bool RemoveTexture(int index);
    void ClearTextureLibrary();

    // Material-texture assignment
    bool AssignTextureToMaterial(int materialIndex, TextureType textureType, int textureIndex);
    bool RemoveTextureFromMaterial(int materialIndex, TextureType textureType);

    // Selection
    void SelectMaterial(int index);
    void SelectTexture(int index);
    int GetSelectedMaterial() const { return m_selectedMaterial; }
    int GetSelectedTexture() const { return m_selectedTexture; }

    // Getters
    MaterialProperties* GetMaterial(int index);
    const MaterialProperties* GetMaterial(int index) const;
    MaterialTexture* GetTexture(int index);
    const MaterialTexture* GetTexture(int index) const;
    size_t GetMaterialCount() const { return m_materials.size(); }
    size_t GetTextureCount() const { return m_textureLibrary.size(); }

    // Material preview
    void GenerateMaterialPreview(int materialIndex);
    void RenderMaterialPreview(const Vector3& position, float scale = 1.0f);

    // Utility functions
    std::vector<std::string> GetSupportedTextureFormats() const;
    bool IsTextureFormatSupported(const std::string& extension) const;
    std::string GetMaterialInfo(int index) const;
    std::string GetTextureInfo(int index) const;

    // File operations
    bool SaveMaterialLibrary(const std::string& filePath);
    bool LoadMaterialLibrary(const std::string& filePath);
    bool ExportMaterial(int index, const std::string& filePath);

    // Rendering
    void Render();
    void RenderMaterialPanel();
    void RenderTexturePanel();
    void RenderPreviewPanel();

    // Material application
    void ApplyMaterialToObject(MapObject& object, int materialIndex);
    MaterialProperties GetMaterialForObject(const MapObject& object) const;

private:
    // Helper functions
    void SetupDefaultMaterials();
    void GeneratePreviewMesh();
    void UpdateMaterialShader(int materialIndex);
    bool LoadTextureInternal(const std::string& filePath, MaterialTexture& texture);
    void UnloadTextureInternal(MaterialTexture& texture);

    // Material validation
    bool ValidateMaterial(const MaterialProperties& material) const;
    bool ValidateTexture(const MaterialTexture& texture) const;

    // Serialization
    std::string MaterialToJson(const MaterialProperties& material) const;
    std::string TextureToJson(const MaterialTexture& texture) const;
    MaterialProperties JsonToMaterial(const std::string& json) const;
    MaterialTexture JsonToTexture(const std::string& json) const;
};

#endif // MATERIALEDITOR_H
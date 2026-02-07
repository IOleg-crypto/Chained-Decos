#include "model_asset.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "raylib.h"
#include "shader_asset.h"
#include "texture_asset.h"
#include <filesystem>
#include <functional>
#include <unordered_map>

#include "engine/scene/project.h"
#include "asset_manager.h"
#include <iostream>
#include <algorithm>
#include <cstring>

#include "cgltf.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
{

// ============================================================================
// glTF Parser - simple and clean!
// ============================================================================
struct GLTFParserContext {
    ModelAsset::PendingModelData& data;
    cgltf_data* gltf;
};

    static void ProcessGLTFNode(const cgltf_node* node, const glm::mat4& parentTransform, GLTFParserContext& ctx)
    {
        glm::mat4 localTransform = glm::mat4(1.0f);
        cgltf_node_transform_local(node, glm::value_ptr(localTransform));
        
        glm::mat4 worldTransform = parentTransform * localTransform;

        if (node->mesh) {
            for (size_t i = 0; i < node->mesh->primitives_count; ++i) {
                const cgltf_primitive& primitive = node->mesh->primitives[i];
                ModelAsset::RawMesh rawMesh;

                // Material
                if (primitive.material) {
                    for (size_t m = 0; m < ctx.gltf->materials_count; ++m) {
                        if (&ctx.gltf->materials[m] == primitive.material) {
                            rawMesh.materialIndex = (int)m;
                            break;
                        }
                    }
                }

                // Primitive geometry
                cgltf_accessor* positionAccessor = nullptr;
                cgltf_accessor* normalAccessor = nullptr;
                cgltf_accessor* texCoordAccessor = nullptr;

                for (size_t k = 0; k < primitive.attributes_count; ++k) {
                    const cgltf_attribute& attribute = primitive.attributes[k];
                    if (attribute.type == cgltf_attribute_type_position) positionAccessor = attribute.data;
                    else if (attribute.type == cgltf_attribute_type_normal) normalAccessor = attribute.data;
                    else if (attribute.type == cgltf_attribute_type_texcoord) texCoordAccessor = attribute.data;
                }

                if (positionAccessor) {
                    size_t count = positionAccessor->count;
                    rawMesh.vertices.resize(count * 3);
                    cgltf_accessor_unpack_floats(positionAccessor, rawMesh.vertices.data(), rawMesh.vertices.size());

                    // Apply world transform to positions
                    for (size_t v = 0; v < count; ++v) {
                        glm::vec4 position = worldTransform * glm::vec4(rawMesh.vertices[v*3], rawMesh.vertices[v*3+1], rawMesh.vertices[v*3+2], 1.0f);
                        rawMesh.vertices[v*3] = position.x;
                        rawMesh.vertices[v*3+1] = position.y;
                        rawMesh.vertices[v*3+2] = position.z;
                    }
                }

                if (normalAccessor) {
                    size_t count = normalAccessor->count;
                    rawMesh.normals.resize(count * 3);
                    cgltf_accessor_unpack_floats(normalAccessor, rawMesh.normals.data(), rawMesh.normals.size());

                    // Apply rotation only (normal matrix)
                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldTransform)));
                    for (size_t v = 0; v < count; ++v) {
                        glm::vec3 n = normalMatrix * glm::vec3(rawMesh.normals[v*3], rawMesh.normals[v*3+1], rawMesh.normals[v*3+2]);
                        n = glm::normalize(n);
                        rawMesh.normals[v*3] = n.x;
                        rawMesh.normals[v*3+1] = n.y;
                        rawMesh.normals[v*3+2] = n.z;
                    }
                }

                if (texCoordAccessor) {
                    rawMesh.texcoords.resize(texCoordAccessor->count * 2);
                    cgltf_accessor_unpack_floats(texCoordAccessor, rawMesh.texcoords.data(), rawMesh.texcoords.size());
                    // glTF texcoords are typically top-left, Raylib/GL expects bottom-left
                    for (size_t v = 1; v < rawMesh.texcoords.size(); v += 2) {
                        rawMesh.texcoords[v] = 1.0f - rawMesh.texcoords[v];
                    }
                }

                if (primitive.indices) {
                    rawMesh.indices.resize(primitive.indices->count);
                    for (size_t idx = 0; idx < primitive.indices->count; ++idx) {
                        rawMesh.indices[idx] = (unsigned short)cgltf_accessor_read_index(primitive.indices, idx);
                    }
                }

                ctx.data.meshes.push_back(std::move(rawMesh));
            }
        }

        for (size_t i = 0; i < node->children_count; ++i) {
            ProcessGLTFNode(node->children[i], worldTransform, ctx);
        }
    }

    static bool ParseGLTF(const std::string& path, ModelAsset::PendingModelData& data)
    {
        cgltf_options options = {};
        cgltf_data* gltf = nullptr;
        
        if (cgltf_parse_file(&options, path.c_str(), &gltf) != cgltf_result_success) return false;
        if (cgltf_load_buffers(&options, gltf, path.c_str()) != cgltf_result_success) {
            cgltf_free(gltf);
            return false;
        }

        std::filesystem::path modelDir = std::filesystem::path(path).parent_path();

        // Load materials
        for (size_t i = 0; i < gltf->materials_count; ++i) {
            ModelAsset::RawMaterial material;
            const auto& gltf_material = gltf->materials[i];
            
            if (gltf_material.has_pbr_metallic_roughness) {
                const auto& pbr = gltf_material.pbr_metallic_roughness;
                if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image && pbr.base_color_texture.texture->image->uri) {
                    material.albedoPath = (modelDir / pbr.base_color_texture.texture->image->uri).string();
                }
                material.albedoColor.r = (unsigned char)(pbr.base_color_factor[0] * 255);
                material.albedoColor.g = (unsigned char)(pbr.base_color_factor[1] * 255);
                material.albedoColor.b = (unsigned char)(pbr.base_color_factor[2] * 255);
                material.albedoColor.a = (unsigned char)(pbr.base_color_factor[3] * 255);
            }
            data.materials.push_back(material);
        }

        // Process nodes starting from scenes
        GLTFParserContext ctx = { data, gltf };
        for (size_t i = 0; i < gltf->scenes_count; ++i) {
            for (size_t j = 0; j < gltf->scenes[i].nodes_count; ++j) {
                ProcessGLTFNode(gltf->scenes[i].nodes[j], glm::mat4(1.0f), ctx);
            }
        }

        cgltf_free(gltf);
        return true;
    }

    // ============================================================================
    // OBJ Parser
    // ============================================================================
    static bool ParseOBJ(const std::string& path, ModelAsset::PendingModelData& data)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string errorMessage;

        std::filesystem::path modelDir = std::filesystem::path(path).parent_path();
        std::string materialDirectory = modelDir.string() + "/";

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &errorMessage, path.c_str(), materialDirectory.c_str(), true)) {
            CH_CORE_ERROR("OBJ failed: {}", errorMessage);
            return false;
        }

        // Materials
        for (const auto& material : materials) {
            ModelAsset::RawMaterial rawMaterial;
            rawMaterial.albedoColor.r = (unsigned char)(material.diffuse[0] * 255);
            rawMaterial.albedoColor.g = (unsigned char)(material.diffuse[1] * 255);
            rawMaterial.albedoColor.b = (unsigned char)(material.diffuse[2] * 255);
            rawMaterial.albedoColor.a = 255;
            
            if (!material.diffuse_texname.empty()) {
                rawMaterial.albedoPath = (modelDir / material.diffuse_texname).string();
            }
            data.materials.push_back(rawMaterial);
        }

        // Meshes
        for (const auto& shape : shapes) {
            ModelAsset::RawMesh rawMesh;
            
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                int materialId = (f < shape.mesh.material_ids.size()) ? shape.mesh.material_ids[f] : 0;
                rawMesh.materialIndex = materialId;

                for (size_t v = 0; v < 3; ++v) {
                    tinyobj::index_t index = shape.mesh.indices[f * 3 + v];
                    
                    rawMesh.vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
                    rawMesh.vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
                    rawMesh.vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

                    if (index.normal_index >= 0) {
                        rawMesh.normals.push_back(attrib.normals[3 * index.normal_index + 0]);
                        rawMesh.normals.push_back(attrib.normals[3 * index.normal_index + 1]);
                        rawMesh.normals.push_back(attrib.normals[3 * index.normal_index + 2]);
                    }

                    if (index.texcoord_index >= 0) {
                        rawMesh.texcoords.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                        rawMesh.texcoords.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
                    }
                    
                    rawMesh.indices.push_back((unsigned short)(f * 3 + v));
                }
            }
            
            data.meshes.push_back(std::move(rawMesh));
        }

        return true;
    }

    // ============================================================================
    // Main API
    // ============================================================================

    std::shared_ptr<ModelAsset> ModelAsset::Load(const std::string &path)
    {
        auto asset = std::make_shared<ModelAsset>();
        asset->SetPath(path);
        asset->LoadFromFile(path);
        if (asset->GetState() == AssetState::Ready || asset->GetState() == AssetState::Loading) return asset;
        return nullptr;
    }

    void ModelAsset::LoadFromFile(const std::string &path)
    {
        if (path.empty()) {
            SetState(AssetState::Failed);
            return;
        }

        // Procedural (cube, sphere, etc.)
        if (path.starts_with(":")) {
            m_PendingData.isValid = true;
            m_HasPendingData = true;
            SetPath(path);
            return;
        }

        // File check
        std::filesystem::path fullPath(path);
        if (!std::filesystem::exists(fullPath)) {
            CH_CORE_ERROR("Model not found: {}", path);
            SetState(AssetState::Failed);
            return;
        }

        // Parse based on extension
        std::string absolutePath = std::filesystem::absolute(fullPath).string();
        std::string extension = fullPath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        bool success = false;
        if (extension == ".gltf" || extension == ".glb") {
            success = ParseGLTF(absolutePath, m_PendingData);
        } 
        else if (extension == ".obj") {
            success = ParseOBJ(absolutePath, m_PendingData);
        } 
        else {
            CH_CORE_ERROR("Unsupported format: {}", extension);
            SetState(AssetState::Failed);
            return;
        }

        if (success) {
            m_PendingData.fullPath = absolutePath;
            m_PendingData.isValid = true;
            m_HasPendingData = true;
            SetPath(path);
            SetState(AssetState::Loading); // Ready for GPU upload
        } else {
            CH_CORE_ERROR("Failed to parse: {}", path);
            SetState(AssetState::Failed);
        }
    }

    void ModelAsset::UploadToGPU()
    {
        if (!m_HasPendingData || !m_PendingData.isValid)
            return;
        
        // Handle procedural models
        if (GetPath().starts_with(":"))
        {
            std::lock_guard<std::mutex> lock(m_ModelMutex);
            this->m_Model = GenerateProceduralModel(GetPath());
            if (this->m_Model.meshCount > 0)
            {
                SetState(AssetState::Ready);
            }
            else 
            {
                SetState(AssetState::Failed);
            }
            
            m_HasPendingData = false;
            m_PendingData.isValid = false;
            return;
        }
        
        // Convert Raw Data to Raylib objects
        CH_CORE_INFO("ModelAsset: Creating Raylib model for '{}' ({} meshes, {} materials)", GetPath(), m_PendingData.meshes.size(), m_PendingData.materials.size());
        
        Model model = {0};
        model.meshCount = (int)m_PendingData.meshes.size();
        if (model.meshCount > 0) {
            model.meshes = (Mesh*)RL_CALLOC(model.meshCount, sizeof(Mesh));
            model.meshMaterial = (int*)RL_CALLOC(model.meshCount, sizeof(int));
        }
        
        model.materialCount = (int)(m_PendingData.materials.empty() ? 1 : m_PendingData.materials.size());
        model.materials = (Material*)RL_CALLOC(model.materialCount, sizeof(Material));
        
        // Load Materials
        auto project = Project::GetActive();
        m_PendingTextures.clear(); // Clear any old pending ones

        for (int i = 0; i < model.materialCount; ++i) {
            model.materials[i] = LoadMaterialDefault();
            
            if (!m_PendingData.materials.empty()) {
                const auto& rawMaterial = m_PendingData.materials[i];
                model.materials[i].maps[MATERIAL_MAP_ALBEDO].color = rawMaterial.albedoColor;
                
                if (!rawMaterial.albedoPath.empty() && project) {
                    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.albedoPath);
                    if (textureAsset && textureAsset->IsReady()) {
                        model.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = textureAsset->GetTexture();
                        m_Textures.push_back(textureAsset);
                        CH_CORE_INFO("ModelAsset: Loaded texture '{}' for material {}", rawMaterial.albedoPath, i);
                    }
                    else if (textureAsset) {
                        CH_CORE_WARN("ModelAsset: Texture '{}' exists but not ready (state: {}). Adding to pending.", 
                            rawMaterial.albedoPath, (int)textureAsset->GetState());
                        m_PendingTextures.push_back({i, rawMaterial.albedoPath});
                    }
                    else {
                        CH_CORE_WARN("ModelAsset: Failed to load texture '{}'", rawMaterial.albedoPath);
                    }
                }
            }
        }

        // Load Meshes
        for (int i = 0; i < model.meshCount; ++i) {
            const auto& rawMesh = m_PendingData.meshes[i];
            Mesh mesh = {0};
            mesh.vertexCount = (int)rawMesh.vertices.size() / 3;
            mesh.triangleCount = (int)rawMesh.indices.size() / 3;
            
            if (mesh.vertexCount > 0) {
                mesh.vertices = (float*)RL_MALLOC(rawMesh.vertices.size() * sizeof(float));
                std::memcpy(mesh.vertices, rawMesh.vertices.data(), rawMesh.vertices.size() * sizeof(float));
                
                if (!rawMesh.texcoords.empty()) {
                    mesh.texcoords = (float*)RL_MALLOC(rawMesh.texcoords.size() * sizeof(float));
                    std::memcpy(mesh.texcoords, rawMesh.texcoords.data(), rawMesh.texcoords.size() * sizeof(float));
                }

                if (!rawMesh.normals.empty()) {
                    mesh.normals = (float*)RL_MALLOC(rawMesh.normals.size() * sizeof(float));
                    std::memcpy(mesh.normals, rawMesh.normals.data(), rawMesh.normals.size() * sizeof(float));
                }

                if (!rawMesh.indices.empty()) {
                    mesh.indices = (unsigned short*)RL_MALLOC(rawMesh.indices.size() *sizeof(unsigned short));
                    std::memcpy(mesh.indices, rawMesh.indices.data(), rawMesh.indices.size() * sizeof(unsigned short));
                }

                UploadMesh(&mesh, false);
            }
            
            model.meshes[i] = mesh;
            model.meshMaterial[i] = (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < model.materialCount) ? rawMesh.materialIndex : 0;
        }

        model.transform = MatrixIdentity();

        // Lock and transfer
        {
            std::lock_guard<std::mutex> lock(m_ModelMutex);
            m_Model = model;
        }
        
        m_PendingData = PendingModelData();
        m_HasPendingData = false;
        
        SetState(AssetState::Ready);
        CH_CORE_INFO("ModelAsset: GPU upload completed for '{}'", GetPath());
    }

    Model ModelAsset::GenerateProceduralModel(const std::string &type)
    {
        static const std::unordered_map<std::string, std::function<Mesh()>> s_Generators = {
            {":cube:",       []() { return GenMeshCube(1.0f, 1.0f, 1.0f); }},
            {":sphere:",     []() { return GenMeshSphere(0.5f, 16, 16); }},
            {":plane:",      []() { return GenMeshPlane(10.0f, 10.0f, 10, 10); }},
            {":torus:",      []() { return GenMeshTorus(0.2f, 0.4f, 16, 16); }},
            {":cylinder:",   []() { return GenMeshCylinder(0.5f, 1.0f, 16); }},
            {":cone:",       []() { return GenMeshCone(0.5f, 1.0f, 16); }},
            {":knot:",       []() { return GenMeshKnot(0.5f, 0.2f, 16, 128); }},
            {":hemisphere:", []() { return GenMeshHemiSphere(0.5f, 16, 16); }}
        };

        auto it = s_Generators.find(type);
        if (it == s_Generators.end()) return {0};

        Mesh mesh = it->second();
        if (mesh.vertexCount == 0) return {0};

        return LoadModelFromMesh(mesh);
    }

    std::shared_ptr<ModelAsset> ModelAsset::CreateProcedural(const std::string &type)
    {
        Model model = GenerateProceduralModel(type);
        if (model.meshCount == 0) return nullptr;

        auto asset = std::make_shared<ModelAsset>();
        asset->m_Model = model;
        asset->SetPath(type);
        asset->SetState(AssetState::Ready);
        return asset;
    }

    ModelAsset::~ModelAsset()
    {
        if (m_Model.meshCount > 0) ::UnloadModel(m_Model);
        if (m_Animations) ::UnloadModelAnimations(m_Animations, m_AnimationCount);
    }

    void ModelAsset::UpdateAnimation(int animationIndex, int frame)
    {
        if (m_Animations && animationIndex < m_AnimationCount)
        {
            ::UpdateModelAnimation(m_Model, m_Animations[animationIndex], frame);
        }
    }

    void ModelAsset::OnUpdate()
    {
        if (m_PendingTextures.empty()) return;

        auto project = Project::GetActive();
        if (!project) return;

        auto assetManager = project->GetAssetManager();

        std::lock_guard<std::mutex> lock(m_ModelMutex);

        for (auto it = m_PendingTextures.begin(); it != m_PendingTextures.end(); )
        {
            auto textureAsset = assetManager->Get<TextureAsset>(it->path);
            if (textureAsset && textureAsset->IsReady())
            {
                CH_CORE_INFO("ModelAsset: Applying deferred texture '{}' to material {}", it->path, it->materialIndex);
                
                if (it->materialIndex >= 0 && it->materialIndex < m_Model.materialCount)
                {
                    m_Model.materials[it->materialIndex].maps[MATERIAL_MAP_ALBEDO].texture = textureAsset->GetTexture();
                    m_Textures.push_back(textureAsset);
                }
                
                it = m_PendingTextures.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    ModelAnimation *ModelAsset::GetAnimations(int *count)
    {
        if (count) *count = m_AnimationCount;
        return m_Animations;
    }

    BoundingBox ModelAsset::GetBoundingBox() const
    {
        return ::GetModelBoundingBox(m_Model);
    }


    AssetType ModelAsset::GetType() const
    {
        return AssetType::Model;
    }


    Model& ModelAsset::GetModel()
    {
        std::lock_guard<std::mutex> lock(m_ModelMutex);
        return m_Model;
    }

    const Model& ModelAsset::GetModel() const
    {
        std::lock_guard<std::mutex> lock(m_ModelMutex);
        return m_Model;
    }

    int ModelAsset::GetAnimationCount() const
    {
        return m_AnimationCount;
    }

    const std::vector<std::shared_ptr<TextureAsset>> &ModelAsset::GetTextures() const
    {
        return m_Textures;
    }

} // namespace CHEngine


#define TINYOBJLOADER_IMPLEMENTATION
#include "mesh_importer.h"
#include "model_asset.h"
#include "engine/core/log.h"
#include "raylib.h"
#include "raymath.h"
#include "cgltf.h"
#include "tiny_obj_loader.h"
#include <filesystem>
#include <algorithm>

namespace CHEngine
{
    

    static void ProcessGLTFNode(const cgltf_node* node, const Matrix& parentTransform, GLTFParserContext& ctx)
    {
        Matrix localTransform = MatrixIdentity();
        float matrixData[16];
        cgltf_node_transform_local(node, matrixData);
        
        localTransform.m0 = matrixData[0];  localTransform.m4 = matrixData[4];  localTransform.m8 = matrixData[8];   localTransform.m12 = matrixData[12];
        localTransform.m1 = matrixData[1];  localTransform.m5 = matrixData[5];  localTransform.m9 = matrixData[9];   localTransform.m13 = matrixData[13];
        localTransform.m2 = matrixData[2];  localTransform.m6 = matrixData[6];  localTransform.m10 = matrixData[10]; localTransform.m14 = matrixData[14];
        localTransform.m3 = matrixData[3];  localTransform.m7 = matrixData[7];  localTransform.m11 = matrixData[11]; localTransform.m15 = matrixData[15];
        
        Matrix worldTransform = MatrixMultiply(localTransform, parentTransform);

        if (node->mesh) {
            for (size_t i = 0; i < node->mesh->primitives_count; ++i) {
                const cgltf_primitive& primitive = node->mesh->primitives[i];
                RawMesh rawMesh;

                if (primitive.material) {
                    for (size_t m = 0; m < ctx.gltf->materials_count; ++m) {
                        if (&ctx.gltf->materials[m] == primitive.material) {
                            rawMesh.materialIndex = (int)m;
                            break;
                        }
                    }
                }

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

                    for (size_t v = 0; v < count; ++v) {
                        Vector3 position = { rawMesh.vertices[v*3], rawMesh.vertices[v*3+1], rawMesh.vertices[v*3+2] };
                        Vector3 transformedPos = Vector3Transform(position, worldTransform);
                        rawMesh.vertices[v*3] = transformedPos.x;
                        rawMesh.vertices[v*3+1] = transformedPos.y;
                        rawMesh.vertices[v*3+2] = transformedPos.z;
                    }
                }

                if (normalAccessor) {
                    size_t count = normalAccessor->count;
                    rawMesh.normals.resize(count * 3);
                    cgltf_accessor_unpack_floats(normalAccessor, rawMesh.normals.data(), rawMesh.normals.size());

                    Matrix normalMatrix = worldTransform;
                    normalMatrix.m12 = 0; normalMatrix.m13 = 0; normalMatrix.m14 = 0;
                    
                    for (size_t v = 0; v < count; ++v) {
                         Vector3 normal = { rawMesh.normals[v*3], rawMesh.normals[v*3+1], rawMesh.normals[v*3+2] };
                         Vector3 transformedNormal = Vector3Transform(normal, normalMatrix); 
                         transformedNormal = Vector3Normalize(transformedNormal);
                        rawMesh.normals[v*3] = transformedNormal.x;
                        rawMesh.normals[v*3+1] = transformedNormal.y;
                        rawMesh.normals[v*3+2] = transformedNormal.z;
                    }
                }

                if (texCoordAccessor) {
                    rawMesh.texcoords.resize(texCoordAccessor->count * 2);
                    cgltf_accessor_unpack_floats(texCoordAccessor, rawMesh.texcoords.data(), rawMesh.texcoords.size());
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

    static bool ParseGLTF(const std::string& path, PendingModelData& data)
    {
        cgltf_options options = {};
        cgltf_data* gltf = nullptr;
        
        if (cgltf_parse_file(&options, path.c_str(), &gltf) != cgltf_result_success) return false;
        if (cgltf_load_buffers(&options, gltf, path.c_str()) != cgltf_result_success) {
            cgltf_free(gltf);
            return false;
        }

        std::filesystem::path modelDir = std::filesystem::path(path).parent_path();

        for (size_t i = 0; i < gltf->materials_count; ++i) {
            RawMaterial material;
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

            if (gltf_material.has_emissive_strength) {
                material.emissiveIntensity = gltf_material.emissive_strength.emissive_strength;
            }

            if (gltf_material.emissive_texture.texture && gltf_material.emissive_texture.texture->image && gltf_material.emissive_texture.texture->image->uri) {
                material.emissivePath = (modelDir / gltf_material.emissive_texture.texture->image->uri).string();
            }

            material.emissiveColor.r = (unsigned char)(gltf_material.emissive_factor[0] * 255);
            material.emissiveColor.g = (unsigned char)(gltf_material.emissive_factor[1] * 255);
            material.emissiveColor.b = (unsigned char)(gltf_material.emissive_factor[2] * 255);
            material.emissiveColor.a = 255;

            // If we have emissive color but no strength, default to 1.0 (or higher if colors > 1.0 were used in GLTF)
            if (material.emissiveIntensity == 0.0f && (material.emissiveColor.r > 0 || material.emissiveColor.g > 0 || material.emissiveColor.b > 0)) {
                material.emissiveIntensity = 1.0f;
            }

            data.materials.push_back(material);
        }

        GLTFParserContext ctx = { data, gltf };
        for (size_t i = 0; i < gltf->scenes_count; ++i) {
            for (size_t j = 0; j < gltf->scenes[i].nodes_count; ++j) {
                ProcessGLTFNode(gltf->scenes[i].nodes[j], MatrixIdentity(), ctx);
            }
        }

        cgltf_free(gltf);
        return true;
    }

    static bool ParseOBJ(const std::string& path, PendingModelData& data)
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

        for (const auto& material : materials) {
            RawMaterial rawMaterial;
            rawMaterial.albedoColor.r = (unsigned char)(material.diffuse[0] * 255);
            rawMaterial.albedoColor.g = (unsigned char)(material.diffuse[1] * 255);
            rawMaterial.albedoColor.b = (unsigned char)(material.diffuse[2] * 255);
            rawMaterial.albedoColor.a = 255;
            
            if (!material.diffuse_texname.empty()) {
                rawMaterial.albedoPath = (modelDir / material.diffuse_texname).string();
            }
            data.materials.push_back(rawMaterial);
        }

        for (const auto& shape : shapes) {
            RawMesh rawMesh;
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

    std::shared_ptr<ModelAsset> MeshImporter::ImportMesh(const std::filesystem::path& path)
    {
        std::string pathStr = path.string();
        if (pathStr.starts_with(":"))
        {
            auto asset = std::make_shared<ModelAsset>();
            asset->SetPath(pathStr);
            asset->GetModel() = GenerateProceduralModel(pathStr);
            asset->SetState(AssetState::Ready);
            return asset;
        }

        CH_CORE_INFO("MeshImporter: Importing mesh from {}", pathStr);
        auto asset = std::make_shared<ModelAsset>();
        asset->SetPath(pathStr);
        
        auto pendingData = LoadMeshDataFromDisk(path);
        if (pendingData.isValid)
        {
            asset->SetPendingData(pendingData);
            asset->UploadToGPU(); 
        }
        else
        {
            asset->SetState(AssetState::Failed);
        }
        
        return asset;
    }

    PendingModelData MeshImporter::LoadMeshDataFromDisk(const std::filesystem::path& path)
{
    PendingModelData data;
        
        std::string pathStr = path.string();
        if (pathStr.starts_with(":"))
        {
            data.isValid = true;
            return data;
        }

        if (!std::filesystem::exists(path)) return data;

        std::string absolutePath = std::filesystem::absolute(path).string();
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        bool success = false;
        if (extension == ".gltf" || extension == ".glb") success = ParseGLTF(absolutePath, data);
        else if (extension == ".obj") success = ParseOBJ(absolutePath, data);

        if (success) {
            data.fullPath = absolutePath;
            data.isValid = true;
        }
        return data;
    }

    Model MeshImporter::GenerateProceduralModel(const std::string& type)
    {
        Mesh mesh = { 0 };

        if      (type == ":cube:")       mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        else if (type == ":sphere:")     mesh = GenMeshSphere(0.5f, 16, 16);
        else if (type == ":plane:")      mesh = GenMeshPlane(10.0f, 10.0f, 10, 10);
        else if (type == ":torus:")      mesh = GenMeshTorus(0.2f, 0.4f, 16, 16);
        else if (type == ":cylinder:")   mesh = GenMeshCylinder(0.5f, 1.0f, 16);
        else if (type == ":cone:")       mesh = GenMeshCone(0.5f, 1.0f, 16);
        else if (type == ":knot:")       mesh = GenMeshKnot(0.5f, 0.2f, 16, 128);
        else if (type == ":hemisphere:") mesh = GenMeshHemiSphere(0.5f, 16, 16);

        if (mesh.vertexCount == 0) return { 0 };

        return LoadModelFromMesh(mesh);
    }
}

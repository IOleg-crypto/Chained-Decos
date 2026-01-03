#include "inspector_panel.h"
#include "engine/components.h"
#include "engine/project.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>
#include <raymath.h>

namespace CH
{
static void DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                            float columnWidth = 100.0f)
{
    ImGuiIO &io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0]; // TODO: Use actual bold font

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    ImGui::PushStyleColor(ImGuiColorEditFlags_None, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    if (ImGui::Button("X", buttonSize))
        values.x = resetValue;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiColorEditFlags_None, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiColorEditFlags_None, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    if (ImGui::Button("Z", buttonSize))
        values.z = resetValue;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
}

template <typename T, typename UIFunction>
static void DrawComponent(const std::string &name, Entity entity, UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
        ImGuiTreeNodeFlags_FramePadding;

    if (entity.HasComponent<T>())
    {
        auto &component = entity.GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
        float lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void *)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();

        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
                removeComponent = true;

            ImGui::EndPopup();
        }

        if (open)
        {
            uiFunction(component);
            ImGui::TreePop();
        }

        if (removeComponent)
            entity.RemoveComponent<T>();
    }
}

void InspectorPanel::OnImGuiRender(Entity entity)
{
    ImGui::Begin("Inspector");
    if (entity)
    {
        DrawComponents(entity);
    }
    ImGui::End();
}

void InspectorPanel::DrawComponents(Entity entity)
{
    if (entity.HasComponent<TagComponent>())
    {
        auto &tag = entity.GetComponent<TagComponent>().Tag;

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, tag.c_str());
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tag = std::string(buffer);
        }
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);

    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("AddComponent");

    if (ImGui::BeginPopup("AddComponent"))
    {
        if (ImGui::MenuItem("Tag"))
        {
            if (!entity.HasComponent<TagComponent>())
                entity.AddComponent<TagComponent>("New Entity");
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Transform"))
        {
            if (!entity.HasComponent<TransformComponent>())
                entity.AddComponent<TransformComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Model"))
        {
            if (!entity.HasComponent<ModelComponent>())
                entity.AddComponent<ModelComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Box Collider"))
        {
            if (!entity.HasComponent<BoxColliderComponent>())
                entity.AddComponent<BoxColliderComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Spawn Zone"))
        {
            if (!entity.HasComponent<SpawnComponent>())
                entity.AddComponent<SpawnComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Material"))
        {
            if (!entity.HasComponent<MaterialComponent>())
                entity.AddComponent<MaterialComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Skybox"))
        {
            if (!entity.HasComponent<SkyboxComponent>())
                entity.AddComponent<SkyboxComponent>();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopItemWidth();

    DrawComponent<TransformComponent>("Transform", entity,
                                      [](auto &tc)
                                      {
                                          DrawVec3Control("Translation", tc.Translation);
                                          Vector3 rotation =
                                              tc.Rotation; // TODO: Convert to degrees
                                          DrawVec3Control("Rotation", rotation);
                                          tc.Rotation = rotation;
                                          DrawVec3Control("Scale", tc.Scale, 1.0f);
                                      });

    DrawComponent<ModelComponent>(
        "Model", entity,
        [](auto &mc)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, mc.ModelPath.c_str());
            if (ImGui::InputText("Model Path", buffer, sizeof(buffer)))
            {
                mc.ModelPath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Model Files", "obj,glb,gltf,iqm,msh"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        if (!ec)
                            mc.ModelPath = relativePath.string();
                        else
                            mc.ModelPath = fullPath.string();
                    }
                    else
                    {
                        mc.ModelPath = fullPath.string();
                    }
                    NFD_FreePath(outPath);
                }
            }

            float color[4] = {mc.Tint.r / 255.0f, mc.Tint.g / 255.0f, mc.Tint.b / 255.0f,
                              mc.Tint.a / 255.0f};
            if (ImGui::ColorEdit4("Tint", color))
            {
                mc.Tint.r = (unsigned char)(color[0] * 255.0f);
                mc.Tint.g = (unsigned char)(color[1] * 255.0f);
                mc.Tint.b = (unsigned char)(color[2] * 255.0f);
                mc.Tint.a = (unsigned char)(color[3] * 255.0f);
            }
        });

    DrawComponent<BoxColliderComponent>("Box Collider", entity,
                                        [](auto &bc)
                                        {
                                            DrawVec3Control("Offset", bc.Offset);
                                            DrawVec3Control("Size", bc.Size, 1.0f);

                                            bool colliding = bc.IsColliding;
                                            ImGui::Checkbox("Colliding", &colliding);
                                        });

    DrawComponent<SpawnComponent>("Spawn Zone", entity,
                                  [](auto &sc)
                                  {
                                      ImGui::Checkbox("Active", &sc.IsActive);
                                      DrawVec3Control("Zone Size", sc.ZoneSize, 1.0f);
                                  });

    DrawComponent<MaterialComponent>(
        "Material", entity,
        [](auto &mc)
        {
            float color[4] = {mc.AlbedoColor.r / 255.0f, mc.AlbedoColor.g / 255.0f,
                              mc.AlbedoColor.b / 255.0f, mc.AlbedoColor.a / 255.0f};
            if (ImGui::ColorEdit4("Albedo Color", color))
            {
                mc.AlbedoColor.r = (unsigned char)(color[0] * 255.0f);
                mc.AlbedoColor.g = (unsigned char)(color[1] * 255.0f);
                mc.AlbedoColor.b = (unsigned char)(color[2] * 255.0f);
                mc.AlbedoColor.a = (unsigned char)(color[3] * 255.0f);
            }

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, mc.AlbedoPath.c_str());
            if (ImGui::InputText("Albedo Path", buffer, sizeof(buffer)))
            {
                mc.AlbedoPath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("...##Albedo"))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Image Files", "png,jpg,jpeg,bmp,tga"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        if (!ec)
                            mc.AlbedoPath = relativePath.string();
                        else
                            mc.AlbedoPath = fullPath.string();
                    }
                    else
                    {
                        mc.AlbedoPath = fullPath.string();
                    }
                    NFD_FreePath(outPath);
                }
            }
        });

    DrawComponent<SkyboxComponent>(
        "Skybox", entity,
        [](auto &sc)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, sc.TexturePath.c_str());
            if (ImGui::InputText("Texture Path", buffer, sizeof(buffer)))
            {
                sc.TexturePath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("...##Skybox"))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Environment Map", "hdr,png,jpg,jpeg,bmp,tga"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    if (Project::GetActive())
                    {
                        std::filesystem::path assetDir = Project::GetAssetDirectory();
                        std::error_code ec;
                        auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                        if (!ec)
                            sc.TexturePath = relativePath.string();
                        else
                            sc.TexturePath = fullPath.string();
                    }
                    else
                    {
                        sc.TexturePath = fullPath.string();
                    }
                    NFD_FreePath(outPath);
                }
            }

            ImGui::DragFloat("Exposure", &sc.Exposure, 0.05f, 0.0f, 10.0f);
            ImGui::DragFloat("Brightness", &sc.Brightness, 0.05f, -1.0f, 1.0f);
            ImGui::DragFloat("Contrast", &sc.Contrast, 0.05f, 0.0f, 5.0f);
        });
}
} // namespace CH

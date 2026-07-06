#include "project_serializer.h"
#include "engine/core/log.h"
#include "engine/serialization/serialization.h"
#include "editor_settings.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Chained
{
using namespace Serialization;

bool EditorProjectSerializer::Serialize(const std::shared_ptr<Project>& project, const EditorSettings& editorSettings, const std::filesystem::path& filepath)
{
    const auto& config = project->GetConfig();

    YAML::Emitter out;
    out << YAML::BeginMap; // Project
    out << YAML::Key << "Project" << YAML::Value;
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << config.Name;
        out << YAML::Key << "IconPath" << YAML::Value << config.IconPath;
        out << YAML::Key << "StartScene" << YAML::Value << config.StartScene;
        out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();

        SerializePath(out, "ActiveScene", config.ActiveScenePath.string());
        SerializePath(out, "Environment", config.EnvironmentPath.string());

        out << YAML::Key << "BuildScenes" << YAML::Value << YAML::BeginSeq;
        for (const auto& scene : config.BuildScenes)
        {
            out << scene;
        }
        out << YAML::EndSeq;

        out << YAML::Key << "Physics" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Gravity" << YAML::Value << config.Physics.Gravity;
        out << YAML::Key << "FixedTimestep" << YAML::Value << config.Physics.FixedTimestep;
        out << YAML::EndMap;

        out << YAML::Key << "Animation" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "TargetFPS" << YAML::Value << config.Animation.TargetFPS;
        out << YAML::EndMap;

        out << YAML::Key << "Render" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "AmbientIntensity" << YAML::Value << config.Render.AmbientIntensity;
        out << YAML::Key << "DefaultExposure" << YAML::Value << config.Render.DefaultExposure;
        out << YAML::Key << "ShadowResolution" << YAML::Value << config.Render.ShadowResolution;
        out << YAML::Key << "EnableSSAO" << YAML::Value << config.Render.EnableSSAO;
        out << YAML::Key << "EnableBloom" << YAML::Value << config.Render.EnableBloom;
        out << YAML::Key << "AntiAliasingSamples" << YAML::Value << config.Render.AntiAliasingSamples;
        out << YAML::EndMap;

        out << YAML::Key << "Texture" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "GenerateMipmaps" << YAML::Value << config.Texture.GenerateMipmaps;
        out << YAML::Key << "Filter" << YAML::Value << (int)config.Texture.Filter;
        out << YAML::EndMap;

        out << YAML::Key << "Mesh" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ImportMaterials" << YAML::Value << config.Mesh.ImportMaterials;
        out << YAML::Key << "CalculateTangents" << YAML::Value << config.Mesh.CalculateTangents;
        out << YAML::Key << "FlipUVs" << YAML::Value << config.Mesh.FlipUVs;
        out << YAML::EndMap;

        out << YAML::Key << "Window" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Width" << YAML::Value << config.Window.Width;
        out << YAML::Key << "Height" << YAML::Value << config.Window.Height;
        out << YAML::Key << "VSync" << YAML::Value << config.Window.VSync;
        out << YAML::Key << "Resizable" << YAML::Value << config.Window.Resizable;
        out << YAML::EndMap;

        out << YAML::Key << "Audio" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MasterVolume" << YAML::Value << config.Audio.MasterVolume;
        out << YAML::Key << "MusicVolume" << YAML::Value << config.Audio.MusicVolume;
        out << YAML::Key << "SFXVolume" << YAML::Value << config.Audio.SFXVolume;
        out << YAML::EndMap;

        out << YAML::Key << "Runtime" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Fullscreen" << YAML::Value << config.Runtime.Fullscreen;
        out << YAML::Key << "ShowStats" << YAML::Value << config.Runtime.ShowStats;
        out << YAML::Key << "EnableConsole" << YAML::Value << config.Runtime.EnableConsole;
        out << YAML::Key << "TargetFPS" << YAML::Value << config.Runtime.TargetFPS;
        out << YAML::EndMap;

        out << YAML::Key << "Editor" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "CameraMoveSpeed" << YAML::Value << editorSettings.CameraMoveSpeed;
        out << YAML::Key << "CameraRotationSpeed" << YAML::Value << editorSettings.CameraRotationSpeed;
        out << YAML::Key << "CameraBoostMultiplier" << YAML::Value << editorSettings.CameraBoostMultiplier;
        out << YAML::Key << "DisableCameraZoom" << YAML::Value << editorSettings.DisableCameraZoom;
        out << YAML::Key << "ShowGrid" << YAML::Value << editorSettings.ShowGrid;
        out << YAML::Key << "ShowGizmos" << YAML::Value << editorSettings.ShowGizmos;
        out << YAML::Key << "ShowSelectedWireframe" << YAML::Value << editorSettings.ShowSelectedWireframe;
        out << YAML::EndMap;

        out << YAML::Key << "Scripting" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ModuleName" << YAML::Value << config.Scripting.ModuleName;
        out << YAML::Key << "ModuleDirectory" << YAML::Value << config.Scripting.ModuleDirectory.string();
        out << YAML::Key << "AutoLoad" << YAML::Value << config.Scripting.AutoLoad;
        out << YAML::EndMap;

        out << YAML::Key << "BuildConfig" << YAML::Value << (int)config.BuildConfig;

        out << YAML::EndMap;
    }
    out << YAML::EndMap; // Project

    std::ofstream fout(filepath);
    if (fout.is_open())
    {
        fout << out.c_str();
        return true;
    }

    CH_CORE_ERROR("Failed to save project file: {}", filepath.string());
    return false;
}

bool EditorProjectSerializer::Deserialize(const std::shared_ptr<Project>& project, EditorSettings& outEditorSettings, const std::filesystem::path& filepath)
{
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        CH_CORE_ERROR("Failed to open project file: {}", filepath.string());
        return false;
    }

    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    auto projectNode = data["Project"];
    if (!projectNode)
    {
        return false;
    }

    auto& config = project->GetConfig();
    config.Name = projectNode["Name"].as<std::string>();
    if (projectNode["IconPath"])
    {
        config.IconPath = projectNode["IconPath"].as<std::string>();
    }
    config.StartScene = projectNode["StartScene"].as<std::string>();
    config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();

    DeserializePath(projectNode, "Environment", config.EnvironmentPath);
    DeserializePath(projectNode, "ActiveScene", config.ActiveScenePath);

    auto buildScenes = projectNode["BuildScenes"];
    if (buildScenes)
    {
        config.BuildScenes.clear();
        for (auto scene : buildScenes)
        {
            config.BuildScenes.push_back(scene.as<std::string>());
        }
    }

    if (projectNode["Physics"])
    {
        config.Physics.Gravity = projectNode["Physics"]["Gravity"].as<float>();
        if (projectNode["Physics"]["FixedTimestep"])
        {
            config.Physics.FixedTimestep = projectNode["Physics"]["FixedTimestep"].as<float>();
        }
    }

    if (projectNode["Animation"])
    {
        config.Animation.TargetFPS = projectNode["Animation"]["TargetFPS"].as<float>();
    }

    if (projectNode["Render"])
    {
        config.Render.AmbientIntensity = projectNode["Render"]["AmbientIntensity"].as<float>();
        config.Render.DefaultExposure = projectNode["Render"]["DefaultExposure"].as<float>();
        if (projectNode["Render"]["ShadowResolution"]) config.Render.ShadowResolution = projectNode["Render"]["ShadowResolution"].as<int>();
        if (projectNode["Render"]["EnableSSAO"]) config.Render.EnableSSAO = projectNode["Render"]["EnableSSAO"].as<bool>();
        if (projectNode["Render"]["EnableBloom"]) config.Render.EnableBloom = projectNode["Render"]["EnableBloom"].as<bool>();
        if (projectNode["Render"]["AntiAliasingSamples"]) config.Render.AntiAliasingSamples = projectNode["Render"]["AntiAliasingSamples"].as<int>();
    }

    if (projectNode["Texture"])
    {
        if (projectNode["Texture"]["GenerateMipmaps"])
        {
            config.Texture.GenerateMipmaps = projectNode["Texture"]["GenerateMipmaps"].as<bool>();
        }
        if (projectNode["Texture"]["Filter"])
        {
            config.Texture.Filter = (TextureFilter)projectNode["Texture"]["Filter"].as<int>();
        }
    }

    if (projectNode["Mesh"])
    {
        if (projectNode["Mesh"]["ImportMaterials"]) config.Mesh.ImportMaterials = projectNode["Mesh"]["ImportMaterials"].as<bool>();
        if (projectNode["Mesh"]["CalculateTangents"]) config.Mesh.CalculateTangents = projectNode["Mesh"]["CalculateTangents"].as<bool>();
        if (projectNode["Mesh"]["FlipUVs"]) config.Mesh.FlipUVs = projectNode["Mesh"]["FlipUVs"].as<bool>();
    }

    if (projectNode["Window"])
    {
        config.Window.Width = projectNode["Window"]["Width"].as<int>();
        config.Window.Height = projectNode["Window"]["Height"].as<int>();
        config.Window.VSync = projectNode["Window"]["VSync"].as<bool>();
        config.Window.Resizable = projectNode["Window"]["Resizable"].as<bool>();
    }

    if (projectNode["Audio"])
    {
        config.Audio.MasterVolume = projectNode["Audio"]["MasterVolume"].as<float>();
        config.Audio.MusicVolume = projectNode["Audio"]["MusicVolume"].as<float>();
        config.Audio.SFXVolume = projectNode["Audio"]["SFXVolume"].as<float>();
    }

    if (projectNode["Runtime"])
    {
        if (projectNode["Runtime"]["Fullscreen"]) config.Runtime.Fullscreen = projectNode["Runtime"]["Fullscreen"].as<bool>();
        if (projectNode["Runtime"]["ShowStats"]) config.Runtime.ShowStats = projectNode["Runtime"]["ShowStats"].as<bool>();
        if (projectNode["Runtime"]["EnableConsole"]) config.Runtime.EnableConsole = projectNode["Runtime"]["EnableConsole"].as<bool>();
        if (projectNode["Runtime"]["TargetFPS"]) config.Runtime.TargetFPS = projectNode["Runtime"]["TargetFPS"].as<int>();
    }

    if (projectNode["Editor"])
    {
        if (projectNode["Editor"]["CameraMoveSpeed"]) outEditorSettings.CameraMoveSpeed = projectNode["Editor"]["CameraMoveSpeed"].as<float>();
        if (projectNode["Editor"]["CameraRotationSpeed"]) outEditorSettings.CameraRotationSpeed = projectNode["Editor"]["CameraRotationSpeed"].as<float>();
        if (projectNode["Editor"]["CameraBoostMultiplier"]) outEditorSettings.CameraBoostMultiplier = projectNode["Editor"]["CameraBoostMultiplier"].as<float>();
        if (projectNode["Editor"]["DisableCameraZoom"]) outEditorSettings.DisableCameraZoom = projectNode["Editor"]["DisableCameraZoom"].as<bool>();
        if (projectNode["Editor"]["ShowGrid"]) outEditorSettings.ShowGrid = projectNode["Editor"]["ShowGrid"].as<bool>();
        if (projectNode["Editor"]["ShowGizmos"]) outEditorSettings.ShowGizmos = projectNode["Editor"]["ShowGizmos"].as<bool>();
        if (projectNode["Editor"]["ShowSelectedWireframe"]) outEditorSettings.ShowSelectedWireframe = projectNode["Editor"]["ShowSelectedWireframe"].as<bool>();
    }

    if (projectNode["Scripting"])
    {
        config.Scripting.ModuleName = projectNode["Scripting"]["ModuleName"].as<std::string>();
        if (projectNode["Scripting"]["ModuleDirectory"])
        {
            config.Scripting.ModuleDirectory = projectNode["Scripting"]["ModuleDirectory"].as<std::string>();
        }
        if (projectNode["Scripting"]["AutoLoad"])
        {
            config.Scripting.AutoLoad = projectNode["Scripting"]["AutoLoad"].as<bool>();
        }
    }

    if (projectNode["BuildConfig"])
    {
        config.BuildConfig = (Configuration)projectNode["BuildConfig"].as<int>();
    }

    config.ProjectDirectory = filepath.parent_path();

    return true;
}
} // namespace Chained


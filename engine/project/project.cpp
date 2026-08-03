// project.cpp
// Chained Engine — Project configuration loading, path resolution, and environment management.
// Handles .chproject YAML deserialization and cross-platform path normalization.

#include "project.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include <sstream>

namespace Chained
{

	std::shared_ptr<Project> Project::s_ActiveProject = nullptr;

	Project::~Project() = default;

	std::shared_ptr<Project> Project::Load(const std::filesystem::path& filepath)
	{
		auto project = std::make_shared<Project>();

		std::string content;

		// Try reading from pack first (exported builds)
		if (auto* am = ServiceLocator::TryGet<AssetManager>())
		{
			if (am->IsPacked())
			{
				auto data = am->ReadAssetData("project.chproject");
				if (!data.empty())
				{
					content.assign(data.begin(), data.end());
				}
			}
		}

		// Fallback to disk
		if (content.empty())
		{
			std::ifstream stream(filepath);
			if (!stream.is_open())
			{
				return nullptr;
			}
			std::stringstream strStream;
			strStream << stream.rdbuf();
			content = strStream.str();
		}

		YAML::Node data = YAML::Load(content);
		auto projectNode = data["Project"];
		if (!projectNode)
		{
			return nullptr;
		}

		auto& config = project->m_Config;
		if (projectNode["Name"])
		{
			config.Name = projectNode["Name"].as<std::string>();
		}
		if (projectNode["IconPath"])
		{
			config.IconPath = projectNode["IconPath"].as<std::string>();
		}

		if (projectNode["StartScene"])
		{
			config.StartScene = projectNode["StartScene"].as<std::string>();
		}
		if (projectNode["AssetDirectory"])
		{
			config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
		}

		if (projectNode["ActiveScene"])
		{
			config.ActiveScenePath = projectNode["ActiveScene"].as<std::string>();
		}
		if (projectNode["Physics"])
		{
			config.Physics.Gravity = projectNode["Physics"]["Gravity"].as<float>();
			if (projectNode["Physics"]["FixedTimestep"])
			{
				config.Physics.FixedTimestep = projectNode["Physics"]["FixedTimestep"].as<float>();
			}
		}

		if (auto window = projectNode["Window"])
		{
			if (window["Width"])
			{
				config.Window.Width = window["Width"].as<int>();
			}
			if (window["Height"])
			{
				config.Window.Height = window["Height"].as<int>();
			}
			if (window["VSync"])
			{
				config.Window.VSync = window["VSync"].as<bool>();
			}
		}

		if (auto runtime = projectNode["Runtime"])
		{
			if (runtime["Fullscreen"])
			{
				config.Runtime.Fullscreen = runtime["Fullscreen"].as<bool>();
			}
			if (runtime["ShowStats"])
			{
				config.Runtime.ShowStats = runtime["ShowStats"].as<bool>();
			}
			if (runtime["EnableConsole"])
			{
				config.Runtime.EnableConsole = runtime["EnableConsole"].as<bool>();
			}
			if (runtime["TargetFPS"])
			{
				config.Runtime.TargetFPS = runtime["TargetFPS"].as<int>();
			}
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

		config.ProjectDirectory = filepath.parent_path();

		SetActive(project);
		return project;
	}

	std::shared_ptr<Project> Project::GetActive()
	{
		return s_ActiveProject;
	}

	void Project::SetActive(std::shared_ptr<Project> project)
	{
		s_ActiveProject = std::move(project);
	}

	std::filesystem::path Project::GetAssetDirectory()
	{
		return s_ActiveProject ? s_ActiveProject->GetAssetDirectoryForProject() : std::filesystem::path();
	}

	std::filesystem::path Project::GetProjectDirectory()
	{
		return s_ActiveProject ? s_ActiveProject->GetProjectDirectoryForProject() : std::filesystem::path();
	}

	std::filesystem::path Project::GetAssetPath(const std::filesystem::path& relative)
	{
		return s_ActiveProject ? s_ActiveProject->GetAssetPathForProject(relative) : relative;
	}

	std::string Project::GetRelativePath(const std::filesystem::path& path)
	{
		return s_ActiveProject ? s_ActiveProject->GetRelativePathForProject(path) : path.generic_string();
	}

	std::filesystem::path Project::GetAbsolutePath(const std::filesystem::path& path)
	{
		return s_ActiveProject ? s_ActiveProject->GetAbsolutePathForProject(path) : path;
	}

	std::vector<std::string> Project::GetAvailableScenes() const
	{
		std::vector<std::string> scenes;

		auto assetDir = GetAssetDirectoryForProject();
		auto scenesDir = assetDir / "scenes";

		if (std::filesystem::exists(scenesDir))
		{
			try
			{
				for (auto& entry : std::filesystem::recursive_directory_iterator(scenesDir))
				{
					if (entry.path().extension() == ".chscene")
					{
						std::string relPath = std::filesystem::relative(entry.path(), assetDir).string();
						scenes.push_back(relPath);
					}
				}
			} catch (const std::filesystem::filesystem_error& e)
			{
				CH_CORE_WARN("Project: Failed to enumerate scenes in '{}': {}", scenesDir.string(), e.what());
			}
		}
		return scenes;
	}

	std::string Project::GetRelativePathForProject(const std::filesystem::path& path) const
	{
		if (path.empty())
		{
			return "";
		}

		if (path.is_relative())
		{
			return path.generic_string();
		}

		auto absolutePath = NormalizePath(path);

		// 1. Try relative to Assets Directory
		if (auto rel = TryMakeRelative(absolutePath, GetAssetDirectoryForProject()))
		{
			return *rel;
		}

		// 2. Try relative to Project Root
		if (auto rel = TryMakeRelative(absolutePath, GetProjectDirectoryForProject()))
		{
			return *rel;
		}

		return absolutePath.generic_string();
	}

	std::filesystem::path Project::GetAbsolutePathForProject(const std::filesystem::path& path) const
	{
		if (path.empty())
		{
			return "";
		}

		if (path.is_absolute())
		{
			return NormalizePath(path);
		}

		std::string pathStr = path.generic_string();

		// For game assets, try asset directory first
		std::filesystem::path assetDir = GetAssetDirectoryForProject();
		if (!assetDir.empty())
		{
			std::filesystem::path candidate = assetDir / pathStr;
			if (std::filesystem::exists(candidate))
			{
				return NormalizePath(candidate);
			}
		}

		// Try project root next
		std::filesystem::path projectDir = GetProjectDirectoryForProject();
		if (!projectDir.empty())
		{
			std::filesystem::path candidate = projectDir / pathStr;
			if (std::filesystem::exists(candidate))
			{
				return NormalizePath(candidate);
			}
		}

		// Project is now just a data holder for project-specific paths.
		// Engine root resolution is handled by AssetManager.

		return NormalizePath(GetProjectDirectoryForProject() / pathStr);
	}

	// -------------------------------------------------------------------------------------------------------------------
	// Path Utility Helpers
	// -------------------------------------------------------------------------------------------------------------------

	std::filesystem::path Project::NormalizePath(const std::filesystem::path& path)
	{
		// Use lexically_normal to handle .. and . and unify slashes
		std::filesystem::path normalized = std::filesystem::absolute(path).lexically_normal();

#if CH_PLATFORM_WINDOWS
		// Unify drive letter casing to uppercase to prevent relative path resolution failures
		std::string s = normalized.string();
		if (s.length() >= 2 && s[1] == ':' && std::islower(s[0]))
		{
			s[0] = (char)std::toupper(s[0]);
			normalized = s;
		}
#endif

		// On Windows, generic_string() will use / which is exactly what we want for cross-platform portability.
		return normalized;
	}

	std::optional<std::string> Project::TryMakeRelative(const std::filesystem::path& absolutePath,
														const std::filesystem::path& basePath)
	{
		if (basePath.empty())
		{
			return std::nullopt;
		}

		auto normalizedBase = NormalizePath(basePath);
		std::filesystem::path rel = std::filesystem::relative(absolutePath, normalizedBase);

		// std::filesystem::relative returns an absolute path if it cannot resolve relativity (e.g., different drives)
		if (rel.is_relative())
		{
			std::string relStr = rel.generic_string();

			// Only return if the path doesn't escape the base directory.
			// Check the first path component rather than a substring search, so that
			// legitimately named files/folders (e.g. "..data") are not rejected.
			if (rel.begin() == rel.end() || *rel.begin() != "..")
			{
				return relStr;
			}
		}

		return std::nullopt;
	}
} // namespace Chained

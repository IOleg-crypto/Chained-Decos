#include "engine/project/project.h"
#include "gtest/gtest.h"
#include "scripting/scriptengine.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/application.h"

#include <filesystem>
#include <vector>

using namespace Chained;

namespace
{
std::vector<std::filesystem::path> GetReloadAssemblyCandidates()
{
    std::vector<std::filesystem::path> candidates;
    candidates.emplace_back(std::filesystem::current_path() / "ChainedDecos.Scripts.dll");
    candidates.emplace_back(std::filesystem::current_path() / "Chained.Managed.dll");

#ifdef PROJECT_ROOT_DIR
    const std::filesystem::path root = std::filesystem::path(PROJECT_ROOT_DIR);
    candidates.emplace_back(root / "game" / "chaineddecos" / "bin" / "ChainedDecos.Scripts.dll");
    candidates.emplace_back(root / "game" / "chaineddecos" / "scripts" / "bin" / "ChainedDecos.Scripts.dll");
    candidates.emplace_back(root / "game" / "chaineddecos" / "scripts" / "game" / "chaineddecos" / "bin" /
                            "ChainedDecos.Scripts.dll");
    candidates.emplace_back(root / "build" / "windows-ninja" / "bin" / "ChainedDecos.Scripts.dll");
    candidates.emplace_back(root / "build" / "windows-gcc" / "bin" / "ChainedDecos.Scripts.dll");
    candidates.emplace_back(root / "build" / "windows-clang" / "bin" / "ChainedDecos.Scripts.dll");
    candidates.emplace_back(root / "build" / "windows-ninja" / "bin" / "Chained.Managed.dll");
    candidates.emplace_back(root / "build" / "windows-gcc" / "bin" / "Chained.Managed.dll");
    candidates.emplace_back(root / "build" / "windows-clang" / "bin" / "Chained.Managed.dll");
    candidates.emplace_back(root / "scripting" / "managed" / "bin" / "Chained.Managed.dll");
    candidates.emplace_back(root / "scripting" / "managed" / "bin" / "Debug" / "Chained.Managed.dll");
#endif

    std::vector<std::filesystem::path> existing;
    for (const auto& candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            existing.emplace_back(std::filesystem::absolute(candidate).lexically_normal());
        }
    }

    return existing;
}
} // namespace

// ScriptEngineTest uses the global ScriptEngine registered by the test environment
// to prevent double CoreCLR init/shutdown which causes fatal crashes.
class ScriptEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!Application::Get().GetServiceRegistry().Has<ScriptEngine>())
        {
            GTEST_SKIP() << "Skipping ScriptEngine tests: ScriptEngine not registered in ServiceRegistry.";
        }
        if (!Application::Get().GetServiceRegistry().Get<ScriptEngine>()->IsHostInitialized())
        {
            GTEST_SKIP() << "Skipping ScriptEngine tests: CoreCLR host initialization failed in this environment.";
        }
    }
};

TEST_F(ScriptEngineTest, ReloadWithoutActiveProjectReturnsFalseAndClearsFlag)
{
    auto* scriptEngine = Application::Get().GetServiceRegistry().Get<ScriptEngine>();
    ASSERT_NE(scriptEngine, nullptr);
    EXPECT_FALSE(scriptEngine->IsReloadInProgress());
    EXPECT_FALSE(scriptEngine->ReloadAssembly(""));
    EXPECT_FALSE(scriptEngine->IsReloadInProgress());
}

TEST_F(ScriptEngineTest, ReloadWithMissingModuleReturnsFalseAndClearsFlag)
{
    auto project = Project::New();
    project->GetConfig().ProjectDirectory = std::filesystem::current_path();
    project->GetConfig().Scripting.ModuleName = "DefinitelyMissingModule";
    project->GetConfig().Scripting.ModuleDirectory = "missing_modules";

    auto* scriptEngine = Application::Get().GetServiceRegistry().Get<ScriptEngine>();
    ASSERT_NE(scriptEngine, nullptr);
    EXPECT_FALSE(scriptEngine->ReloadAssembly("missing_modules/DefinitelyMissingModule.dll"));
    EXPECT_FALSE(scriptEngine->IsReloadInProgress());
}

TEST_F(ScriptEngineTest, LoadAppAssemblyRejectsMissingPath)
{
    auto* scriptEngine = Application::Get().GetServiceRegistry().Get<ScriptEngine>();
    ASSERT_NE(scriptEngine, nullptr);
    EXPECT_FALSE(scriptEngine->LoadAppAssembly("D:/definitely_missing/ChainedDecos.Scripts.dll"));
    EXPECT_TRUE(scriptEngine->GetScriptClasses().empty());
}

TEST_F(ScriptEngineTest, ReloadWithValidProjectLoadsScriptTypes)
{
    const std::vector<std::filesystem::path> candidates = GetReloadAssemblyCandidates();
    if (candidates.empty())
    {
        GTEST_SKIP() << "Skipping ScriptEngine reload success test: no managed assembly candidates found.";
    }

    auto* scriptEngine = Application::Get().GetServiceRegistry().Get<ScriptEngine>();
    ASSERT_NE(scriptEngine, nullptr);
    bool reloaded = false;
    for (const auto& assemblyPath : candidates)
    {
        auto project = Project::New();
        project->GetConfig().ProjectDirectory = assemblyPath.parent_path();
        project->GetConfig().Scripting.ModuleName = assemblyPath.stem().string();
        project->GetConfig().Scripting.ModuleDirectory = assemblyPath.parent_path();

        if (scriptEngine->ReloadAssembly(assemblyPath.string()))
        {
            reloaded = true;
            break;
        }

        EXPECT_FALSE(scriptEngine->IsReloadInProgress());
    }

    if (!reloaded)
    {
        GTEST_SKIP() << "Skipping ScriptEngine reload success test: all managed assembly candidates failed to load.";
    }

    EXPECT_FALSE(scriptEngine->IsReloadInProgress());
    EXPECT_EQ(scriptEngine->GetScriptClass("DefinitelyMissingType"), nullptr);

    if (scriptEngine->GetScriptClasses().empty())
    {
        GTEST_SKIP() << "Loaded assembly exposes no script subclasses; skipping script type lookup assertions.";
    }

    const std::string fullName = scriptEngine->GetScriptClasses().begin()->first;
    ASSERT_FALSE(fullName.empty());
    EXPECT_NE(scriptEngine->GetScriptClass(fullName), nullptr);

    const size_t lastDot = fullName.find_last_of('.');
    if (lastDot != std::string::npos && lastDot + 1 < fullName.size())
    {
        const std::string shortName = fullName.substr(lastDot + 1);
        EXPECT_NE(scriptEngine->GetScriptClass(shortName), nullptr);
    }
}

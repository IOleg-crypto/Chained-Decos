#include "project_exporter.h"

#include "engine/core/log.h"
#include "engine/core/platform.h"
#include "engine/project/project.h"

#include <algorithm>
#include <atomic>
#include <future>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <pack/reader.hpp>
#include <pack/writer.hpp>

namespace fs = std::filesystem;

namespace Chained
{

	struct ExportCancelledException : public std::exception
	{
		const char* what() const noexcept override
		{
			return "Export cancelled by user.";
		}
	};

	void ProjectExporter::CollectFiles(const fs::path& dir, std::vector<fs::path>& out)
	{
		std::error_code ec;
		fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
		if (ec)
		{
			CH_CORE_ERROR("ProjectExporter: Cannot iterate '{}': {}", dir.string(), ec.message());
			return;
		}
		for (const auto& entry : it)
		{
			if (!entry.is_regular_file())
			{
				continue;
			}

			// Skip 0-byte files (pack reader rejects dataSize == 0)
			std::error_code szEc;
			if (entry.file_size(szEc) == 0)
			{
				continue;
			}

			// Skip build artifacts that shouldn't be packed
			const std::string filename = entry.path().filename().string();
			const std::string ext = entry.path().extension().string();

			// Skip .Up2Date marker files, .pdb, .ilk, .obj, .tlog, .log build intermediates
			if (ext == ".pdb" || ext == ".ilk" || ext == ".obj" || ext == ".tlog" || ext == ".log" ||
				ext == ".Up2Date" || ext == ".FileListAbsolute.txt" || ext == ".lastbuildstate" || ext == ".cache" ||
				ext == ".nupkg" || ext == ".nuget.g.props" || ext == ".nuget.g.targets")
			{
				continue;
			}

			// Skip obj/ and Debug/ and Release/ build output directories
			const auto rel = fs::relative(entry.path(), dir, ec);
			if (!ec)
			{
				std::string relStr = rel.string();
				// Normalize path separators to forward slash for comparison
				std::replace(relStr.begin(), relStr.end(), '\\', '/');
				if (relStr.find("/obj/") != std::string::npos || relStr.find("/Debug/") != std::string::npos ||
					relStr.find("/Release/") != std::string::npos || relStr.find("/x64/") != std::string::npos)
				{
					continue;
				}
				out.push_back(rel);
			}
		}
	}

	bool ProjectExporter::CopyFile(const fs::path& src, const fs::path& dst, std::string& outError)
	{
		std::error_code ec;
		fs::create_directories(dst.parent_path(), ec);
		if (ec)
		{
			outError = "Failed to create directory '" + dst.parent_path().string() + "': " + ec.message();
			return false;
		}
		fs::copy_file(src, dst, fs::copy_options::update_existing, ec);
		if (ec)
		{
			outError = "Failed to copy '" + src.string() + "': " + ec.message();
			return false;
		}
		return true;
	}

	bool ProjectExporter::IsPackStale(const fs::path& packPath, const std::vector<std::string>& fileItemPaths,
									  uint64_t expectedItemCount)
	{
		std::error_code ec;
		if (!fs::exists(packPath, ec))
		{
			return true;
		}

		// A differing item count means files were added or removed, which mtime alone cannot detect.
		try
		{
			pack::Reader reader(packPath);
			if (reader.getItemCount() != expectedItemCount)
			{
				return true;
			}
		} catch (...)
		{
			return true;
		}

		const auto packTime = fs::last_write_time(packPath, ec);
		if (ec)
		{
			return true;
		}

		for (size_t i = 0; i < fileItemPaths.size(); i += 2)
		{
			std::error_code srcEc;
			const auto srcTime = fs::last_write_time(fs::path(fileItemPaths[i]), srcEc);
			if (srcEc || srcTime > packTime)
			{
				return true;
			}
		}

		return false;
	}

	ExportResult ProjectExporter::ExportTo(const fs::path& outputDir, ExportProgressCallback onProgress,
										   const std::atomic<bool>* cancelFlag, bool forceRepack)
	{
		ExportResult result;
		result.OutDir = outputDir;

		const bool outputExisted = fs::exists(outputDir);
		// packPath is resolved once we've read the project config (after project is loaded)
		// For CleanupAndCancel we just wipe all .pack files in outputDir
		fs::path packPath; // set after project config is read

		// Only remove the output directory when this run created it — otherwise a cancel
		// would destroy a previous working export, including the pack we tried to preserve.
		auto CleanupAndCancel = [&](const std::string& phaseLog) -> ExportResult {
			if (!outputExisted)
			{
				std::error_code cleanEc;
				fs::remove_all(outputDir, cleanEc);
			}
			else
			{
				// Remove potentially incomplete/corrupted pack files so next export succeeds
				std::error_code cleanEc;
				for (const auto& entry : fs::directory_iterator(outputDir, cleanEc))
				{
					if (entry.is_regular_file() && entry.path().extension() == ".pack")
					{
						fs::remove(entry.path(), cleanEc);
					}
				}
			}
			result.Cancelled = true;
			CH_CORE_INFO("ProjectExporter: Cancelled {}. {}", phaseLog,
						 outputExisted ? "Kept existing output directory." : "Cleaned output directory.");
			return result;
		};

		auto project = Project::GetActive();
		if (!project)
		{
			result.Error = "No active project. Please open a project before exporting.";
			CH_CORE_ERROR("ProjectExporter: {}", result.Error);
			return result;
		}

		const auto& cfg = project->GetConfig();
		const fs::path projectDir = project->GetConfig().ProjectDirectory;
		const fs::path assetDir = project->GetConfig().ProjectDirectory / project->GetConfig().AssetDirectory;
		const fs::path exeDir = Platform::GetExecutableDirectory();
#ifdef CH_BUILD_PRESET
		const std::string buildPreset = CH_BUILD_PRESET;
#else
		const std::string buildPreset = "";
#endif
#ifdef CH_BUILD_CONFIG
		const std::string buildConfig = CH_BUILD_CONFIG;
#else
		const std::string buildConfig = exeDir.string().find("Debug") != std::string::npos ? "Debug" : "Release";
#endif
		const bool isDebugExport = (buildConfig == "Debug");
		CH_CORE_INFO("ProjectExporter: Build preset='{}', config='{}', exeDir='{}'", buildPreset, buildConfig,
					 exeDir.string());

		// ── 0. Prepare output directory ──────────────────────────────────────────
		std::error_code ec;
		fs::create_directories(outputDir, ec);
		if (ec)
		{
			result.Error = "Could not create output directory: " + ec.message();
			CH_CORE_ERROR("ProjectExporter: {}", result.Error);
			return result;
		}

		// ── 1. Find .chproject file ──────────────────────────────────────────────
		fs::path chprojectFile;
		for (const auto& entry : fs::directory_iterator(projectDir, ec))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".chproject")
			{
				chprojectFile = entry.path();
				break;
			}
		}
		if (chprojectFile.empty())
		{
			result.Error = "No .chproject file found in '" + projectDir.string() + "'";
			CH_CORE_ERROR("ProjectExporter: {}", result.Error);
			return result;
		}

		// ── 2. Collect files to pack ─────────────────────────────────────────────
		std::vector<std::string> fileItemPaths;

		// .chproject → "project.chproject"
		fileItemPaths.push_back(chprojectFile.generic_string());
		fileItemPaths.push_back("project.chproject");

		// assets/ → "assets/..."
		if (fs::exists(assetDir))
		{
			std::vector<fs::path> assetFiles;
			CollectFiles(assetDir, assetFiles);
			for (const auto& rel : assetFiles)
			{
				fileItemPaths.push_back((assetDir / rel).generic_string());
				fileItemPaths.push_back((fs::path("assets") / rel).generic_string());
			}
		}

		// resources/ → "resources/..."
		fs::path resourcesDir = exeDir / "resources";
		if (fs::exists(resourcesDir))
		{
			std::vector<fs::path> resFiles;
			CollectFiles(resourcesDir, resFiles);
			for (const auto& rel : resFiles)
			{
				fileItemPaths.push_back((resourcesDir / rel).generic_string());
				fileItemPaths.push_back((fs::path("resources") / rel).generic_string());
			}
		}
		else
		{
			CH_CORE_WARN("ProjectExporter: resources/ not found at '{}' — shaders/fonts may be missing from pack.",
						 resourcesDir.string());
		}

		const auto& exp = project->GetConfig().Export;
		const bool isRawMode = (exp.Mode == PackMode::Raw);

		// Resolve pack base path from PackName setting (default: "resources")
		const std::string packBaseName = exp.PackName.empty() ? "resources" : exp.PackName;
		packPath = outputDir / (packBaseName + ".pack");

		if (fileItemPaths.size() <= 2)
		{
			result.Error = isRawMode ? "No files to copy (no assets or resources found)."
									 : "No files to pack (no assets or resources found).";
			CH_CORE_ERROR("ProjectExporter: {}", result.Error);
			return result;
		}

		if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
		{
			return CleanupAndCancel("before start");
		}

		uint64_t fileCount = fileItemPaths.size() / 2;
		result.PackedFileCount = fileCount;

		// ── 3. Calculate uncompressed size ───────────────────────────────────────
		result.TotalUncompressedSize = 0;
		for (size_t i = 0; i < fileItemPaths.size(); i += 2)
		{
			std::error_code sizeEc;
			auto sz = fs::file_size(fs::path(fileItemPaths[i]), sizeEc);
			if (!sizeEc)
			{
				result.TotalUncompressedSize += sz;
			}
		}

		// ── 4. PARALLEL EXECUTION: Task A (Packing) & Task B (Copying Binaries) ─

		// --- TASK B: Копіювання EXE, DLL та додаткових папок у фоновому потоці ---
		auto copyBinariesTask = std::async(std::launch::async, [&]() -> bool {
			// 1. Copy Executable
			std::vector<fs::path> exeCandidates;
			std::error_code dirEc;
			for (const auto& f : fs::directory_iterator(exeDir, dirEc))
			{
				if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
				{
					return false;
				}
				if (!f.is_regular_file())
				{
					continue;
				}

				std::string fname = f.path().filename().string();
				if (fname.find("Editor") != std::string::npos || fname.find("editor") != std::string::npos ||
					fname.find("test") != std::string::npos)
				{
					continue;
				}
				if (fname.ends_with(".exe") || (fname.find('.') == std::string::npos && !fname.empty()))
				{
					exeCandidates.push_back(f.path());
				}
			}

			if (!exeCandidates.empty())
			{
				const fs::path& srcExe = exeCandidates[0];
				std::string exeName = cfg.Name + srcExe.extension().string();
				fs::path dstExe = outputDir / exeName;
				std::string copyErr;
				if (!CopyFile(srcExe, dstExe, copyErr))
				{
					CH_CORE_ERROR("ProjectExporter: Failed to copy executable: {}", copyErr);
					return false;
				}
			}

			// 2. Copy DLLs — only from the current build config
			// Always skip MSVC CRT DLLs (wrong toolchain) and build-time tools.
			// In Debug: skip release DLLs (no 'd' suffix). In Release: skip debug DLLs ('d' suffix).
			for (const auto& f : fs::directory_iterator(exeDir, dirEc))
			{
				if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
				{
					return false;
				}
				if (!f.is_regular_file())
				{
					continue;
				}

				const std::string ext = f.path().extension().string();
				if (ext != ".dll" && ext != ".so" && ext != ".dylib")
				{
					continue;
				}

				const std::string fname = f.path().filename().string();

				// Always skip MSVC CRT DLLs (e.g. assimp-vc145-mtd.dll) — wrong toolchain
				if (fname.find("-vc") != std::string::npos)
				{
					continue;
				}

				// Skip build-time generator (not needed at runtime)
				if (fname.find("Generator") != std::string::npos)
				{
					continue;
				}

				// Check if this DLL has a debug suffix (e.g. "assimpd.dll" → 'd' before ".dll")
				bool isDebugDll = fname.size() > 4 && fname[fname.size() - 5] == 'd' && fname[fname.size() - 4] == '.';

				// In Debug, skip release DLLs. In Release, skip debug DLLs.
				if (isDebugExport && !isDebugDll)
				{
					continue; // Debug build — skip release DLLs
				}
				if (!isDebugExport && isDebugDll)
				{
					continue; // Release build — skip debug DLLs
				}

				std::string copyErr;
				CopyFile(f.path(), outputDir / f.path().filename(), copyErr);
			}

			// 3. Copy Subdirectories (skip Generator DLLs in scripts/)
			for (const std::string& subDirName : {"nethost", "dotnet", "scripts"})
			{
				if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
				{
					return false;
				}

				fs::path subSrc = exeDir / subDirName;
				if (fs::exists(subSrc))
				{
					fs::path subDst = outputDir / subDirName;
					std::error_code subEc;
					fs::copy(subSrc, subDst, fs::copy_options::update_existing | fs::copy_options::recursive, subEc);

					// Remove Generator DLLs that were copied recursively
					for (const auto& entry : fs::recursive_directory_iterator(subDst, subEc))
					{
						if (entry.is_regular_file())
						{
							const std::string fname = entry.path().filename().string();
							if (fname.find("Generator") != std::string::npos)
							{
								fs::remove(entry.path(), subEc);
							}
						}
					}
				}
			}

			// 4. Copy Coral runtime config files to root (needed by ScriptEngine)
			// Coral files are at exeDir/ (next to the exe), not in scripts/{Name}/
			fs::path coralManagedDir = exeDir;
			const char* coralConfigs[] = {"Coral.Managed.runtimeconfig.json", "Coral.Managed.deps.json",
										  "Coral.Managed.pdb"};
			for (const auto& name : coralConfigs)
			{
				fs::path src = coralManagedDir / name;
				if (fs::exists(src))
				{
					std::string copyErr;
					if (!CopyFile(src, outputDir / name, copyErr))
					{
						CH_CORE_ERROR("ProjectExporter: Failed to copy Coral file '{}': {}", name, copyErr);
					}
				}
				else
				{
					CH_CORE_WARN("ProjectExporter: Coral file '{}' not found in '{}'", name, coralManagedDir.string());
				}
			}

			return true;
		});

		// --- TASK A: Пакування або копіювання ресурсів у головному потоці ---
		bool packSuccess = false;

		if (isRawMode)
		{
			// Raw mode: copy files directly into outputDir, preserving assets/ and resources/ structure.
			// resources.pack is not created; the runtime falls back to the filesystem automatically.
			CH_CORE_INFO("ProjectExporter: Raw mode — copying {} files to '{}'.", fileCount, outputDir.string());

			// Copy .chproject with original name so runtime can discover it by {AppName}.chproject
			auto chProjFile = project->GetConfig().ProjectDirectory / (cfg.Name + ".chproject");
			if (!fs::exists(chProjFile))
			{
				for (const auto& entry : fs::directory_iterator(project->GetConfig().ProjectDirectory))
				{
					if (entry.is_regular_file() && entry.path().extension() == ".chproject")
					{
						chProjFile = entry.path();
						break;
					}
				}
			}
			if (fs::exists(chProjFile))
			{
				std::string copyErr;
				if (!CopyFile(chProjFile, outputDir / (cfg.Name + ".chproject"), copyErr))
				{
					CH_CORE_ERROR("ProjectExporter: Failed to copy .chproject: {}", copyErr);
				}
			}

			for (size_t i = 0; i < fileItemPaths.size(); i += 2)
			{
				if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
				{
					copyBinariesTask.wait();
					return CleanupAndCancel("during raw copy");
				}

				const fs::path srcFile(fileItemPaths[i]);
				const fs::path dstFile = outputDir / fileItemPaths[i + 1];
				std::string copyErr;
				if (!CopyFile(srcFile, dstFile, copyErr))
				{
					CH_CORE_ERROR("ProjectExporter: Raw copy failed: {}", copyErr);
					copyBinariesTask.wait();
					CleanupAndCancel("due to raw copy error");
					result.Error = copyErr;
					return result;
				}

				if (onProgress)
				{
					onProgress((i / 2) + 1, fileCount, fileItemPaths[i + 1]);
				}
			}
			result.PackSkipped = true; // no .pack file produced
			result.PackFileSize = 0;
			packSuccess = true;
		}
		else if (!forceRepack && !IsPackStale(packPath, fileItemPaths, fileCount))
		{
			CH_CORE_INFO("ProjectExporter: {}.pack is up to date ({} items) — skipping repack.", packBaseName,
						 fileCount);
			result.PackSkipped = true;
			packSuccess = true;
		}
		else
		{
			bool preferSpeed = (exp.Mode == PackMode::Fast);
			float threshold = (exp.Mode == PackMode::Max) ? 0.0f : exp.ZipThreshold;

			// --- Build chunks -------------------------------------------------------
			// Each entry in fileItemPaths is: [srcPath, packKey, srcPath, packKey, ...]
			// We split by uncompressed (on-disk) file size per chunk.
			// SplitSizeMB == 0 → one chunk containing all files.
			struct Chunk
			{
				std::vector<std::string> items; // flat [srcPath, packKey, ...]
				uint64_t itemCount = 0;
			};

			std::vector<Chunk> chunks;
			{
				const uint64_t limitBytes =
					exp.SplitSizeMB > 0 ? static_cast<uint64_t>(exp.SplitSizeMB) * 1024 * 1024 : UINT64_MAX;

				chunks.emplace_back();
				uint64_t chunkBytes = 0;

				for (size_t i = 0; i < fileItemPaths.size(); i += 2)
				{
					const std::string& srcPath = fileItemPaths[i];
					std::error_code sizeEc;
					uint64_t fileBytes = static_cast<uint64_t>(fs::file_size(srcPath, sizeEc));
					if (sizeEc)
					{
						fileBytes = 0;
					}

					// Start a new chunk if this file would push the current chunk over the limit
					// (always put at least one file per chunk to avoid infinite loops)
					if (chunkBytes > 0 && chunkBytes + fileBytes > limitBytes)
					{
						chunks.emplace_back();
						chunkBytes = 0;
					}

					chunks.back().items.push_back(fileItemPaths[i]);
					chunks.back().items.push_back(fileItemPaths[i + 1]);
					++chunks.back().itemCount;
					chunkBytes += fileBytes;
				}
			}

			// Track the global item offset for progress reporting across all chunks
			uint64_t globalItemOffset = 0;

			struct PackCtx
			{
				const std::vector<std::string>& chunkItems; // items for this chunk
				uint64_t chunkOffset;						// first item index in global list
				uint64_t totalItems;						// total items across all chunks
				ExportProgressCallback& cb;
				const std::atomic<bool>* cancelFlag;
			};

			try
			{
				for (size_t chunkIdx = 0; chunkIdx < chunks.size(); ++chunkIdx)
				{
					const Chunk& chunk = chunks[chunkIdx];
					if (chunk.itemCount == 0)
					{
						continue;
					}

					// Build pack path: {name}.pack, {name}_1.pack, {name}_2.pack ...
					fs::path chunkPackPath;
					if (chunkIdx == 0)
					{
						chunkPackPath = packPath; // == outputDir / "{packBaseName}.pack"
					}
					else
					{
						chunkPackPath = outputDir / (packBaseName + "_" + std::to_string(chunkIdx) + ".pack");
					}

					std::vector<const char*> rawPaths;
					rawPaths.reserve(chunk.items.size());
					for (const auto& s : chunk.items)
					{
						rawPaths.push_back(s.c_str());
					}

					PackCtx ctx{chunk.items, globalItemOffset, fileCount, onProgress, cancelFlag};

					OnPackFile cCallback = [](uint64_t itemIndex, void* arg) {
						auto* ctx = static_cast<PackCtx*>(arg);

						if (ctx->cancelFlag && ctx->cancelFlag->load(std::memory_order_relaxed))
						{
							throw ExportCancelledException();
						}

						if (ctx->cb)
						{
							uint64_t globalPacked = ctx->chunkOffset + itemIndex + 1;
							const std::string& itemPath = ctx->chunkItems[itemIndex * 2 + 1];
							ctx->cb(globalPacked, ctx->totalItems, itemPath);
						}
					};

					CH_CORE_INFO("ProjectExporter: Packing chunk {}/{} → '{}' ({} items)", chunkIdx + 1, chunks.size(),
								 chunkPackPath.filename().string(), chunk.itemCount);

					pack::Writer::pack(chunkPackPath, chunk.itemCount, rawPaths.data(), exp.DataVersion, threshold,
									   preferSpeed, false, cCallback, &ctx);

					globalItemOffset += chunk.itemCount;
				}

				packSuccess = true;

				if (chunks.size() > 1)
				{
					CH_CORE_INFO("ProjectExporter: Created {} pack chunks.", chunks.size());
				}
			} catch (const ExportCancelledException&)
			{
				copyBinariesTask.wait();
				return CleanupAndCancel("during packing process");
			} catch (const pack::Error& err)
			{
				copyBinariesTask.wait();
				CleanupAndCancel("due to pack error");
				result.Error = "Pack failed: " + std::string(err.what());
				CH_CORE_ERROR("ProjectExporter: {}", result.Error);
				return result;
			}
		}

		// Чекаємо завершення копіювання файлів
		bool copySuccess = copyBinariesTask.get();

		// Перевірка на скасування після обох завдань
		if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
		{
			return CleanupAndCancel("after execution tasks");
		}

		if (!copySuccess || !packSuccess)
		{
			return CleanupAndCancel("due to task failure");
		}

		// ── 5. Get Pack File Size ────────────────────────────────────────────────
		if (!isRawMode)
		{
			// Sum up all .pack files in outputDir
			std::error_code sizeEc;
			for (const auto& entry : fs::directory_iterator(outputDir, sizeEc))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".pack")
				{
					result.PackFileSize += static_cast<uint64_t>(fs::file_size(entry.path(), sizeEc));
				}
			}
		}

		result.Success = true;
		if (isRawMode)
		{
			CH_CORE_INFO("ProjectExporter: Export complete (Raw) → '{}' ({} files copied)", outputDir.string(),
						 result.PackedFileCount);
		}
		else
		{
			CH_CORE_INFO("ProjectExporter: Export complete → '{}' ({} {}, {} MB pack)", outputDir.string(),
						 result.PackedFileCount, result.PackSkipped ? "reused" : "packed",
						 result.PackFileSize / (1024 * 1024));
		}
		return result;
	}

} // namespace Chained
import argparse
import os
import shutil
import subprocess
from pathlib import Path
from typing import Optional

CORAL_ARTIFACTS = [
    "Coral.Managed.dll",
    "Coral.Managed.runtimeconfig.json",
    "Coral.Managed.deps.json",
    "Coral.Managed.pdb",
]
PROPS_TEMPLATE = (
    "<Project>\n"
    "  <PropertyGroup>\n"
    "    <CoralManagedDir>$(MSBuildThisFileDirectory){relative_coral_dir}</CoralManagedDir>\n"
    "  </PropertyGroup>\n"
    "</Project>\n"
)
BUILD_CONFIGS = ["Debug", "Release", "RelWithDebInfo"]


# ── Helpers ───────────────────────────────────────────────────────────────────

def find_dotnet() -> Optional[str]:
    if dotnet_root := os.environ.get("DOTNET_ROOT"):
        for name in ("dotnet.exe", "dotnet"):
            if (candidate := Path(dotnet_root) / name).exists():
                return str(candidate)
    if dotnet := shutil.which("dotnet"):
        return dotnet
    common = [
        "C:/Program Files/dotnet/dotnet.exe",
        "C:/Program Files (x86)/dotnet/dotnet.exe",
        "/usr/bin/dotnet",
        "/usr/share/dotnet/dotnet",
    ]
    return next((str(p) for path in common if (p := Path(path)).exists()), None)


def write_if_changed(path: Path, content: str) -> None:
    if not (path.exists() and path.read_text(encoding="utf-8") == content):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")


# ── Commands ──────────────────────────────────────────────────────────────────

def build_managed(
    project: Path,
    output_dir: Path,
    coral_dir: Path,
    configuration: str,
    copy_coral: bool,
    parallel: bool,
    write_props: bool,
) -> None:
    if not (dotnet := find_dotnet()):
        raise FileNotFoundError("Could not find dotnet. Set DOTNET_ROOT or add dotnet to PATH.")

    if write_props:
        relative = os.path.normpath(os.path.relpath(coral_dir, start=project.parent))
        write_if_changed(
            project.parent / "ManagedDependencies.props",
            PROPS_TEMPLATE.format(relative_coral_dir=relative),
        )

    output_dir.mkdir(parents=True, exist_ok=True)
    command = [
        dotnet, "build", str(project),
        "-c", configuration,
        "--output", str(output_dir),
        f"-p:CoralManagedDir={coral_dir}",
    ]
    if parallel:
        command.append("-m")

    subprocess.run(command, cwd=str(project.parent), check=True)

    if copy_coral:
        for name in CORAL_ARTIFACTS:
            if not (src := coral_dir / name).exists():
                raise FileNotFoundError(f"Missing Coral artifact: {src}")
            shutil.copy2(src, output_dir / name)


def _sync_dir(src: Path, dst: Path) -> None:
    """Replaces dst with a fresh copy of src."""
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src, dst)
    print(f"  Synced: {src} -> {dst}")


def sync_resources(
    project_root: Path,
    bin_dir: Path,
    config: Optional[str],
    all_configs: bool,
) -> None:
    """
    Copy <project_root>/resources into the correct binary output folder.

    Ninja Multi-Config layout:  build/<preset>/bin/<Config>/
    Single-config layout:       build/<preset>/bin/

    --config Debug        -> syncs into  bin_dir/Debug/resources/
    --all-configs         -> syncs into  every bin_dir/<Config>/resources/ that exists
    (neither)             -> syncs into  bin_dir/resources/ (flat / single-config)
    """
    resources_src = project_root / "resources"
    if not resources_src.exists():
        print(f"Warning: Source resources directory '{resources_src}' does not exist.")
        return

    if all_configs:
        synced = False
        for cfg in BUILD_CONFIGS:
            cfg_bin = bin_dir / cfg
            if cfg_bin.exists():
                _sync_dir(resources_src, cfg_bin / "resources")
                synced = True
        if not synced:
            # Fallback for single-config generators
            _sync_dir(resources_src, bin_dir / "resources")
    elif config:
        # Specific config requested
        _sync_dir(resources_src, bin_dir / config / "resources")
    else:
        # Flat layout (single-config generator or manual call)
        _sync_dir(resources_src, bin_dir / "resources")

    print("Success: resources synced!")


# ── CLI ───────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Build helper entrypoint for Chained Decos")
    subs = parser.add_subparsers(dest="command", required=True)

    b = subs.add_parser("build-managed", help="Build a managed C# project")
    b.add_argument("--project",       required=True)
    b.add_argument("--output",        required=True)
    b.add_argument("--coral-dir",     required=True)
    b.add_argument("--configuration", default="Debug")
    b.add_argument("--copy-coral",    action="store_true")
    b.add_argument("--write-props",   action="store_true")
    b.add_argument("--parallel",      action="store_true")

    s = subs.add_parser("sync-resources", help="Copy engine resources into a binary directory")
    s.add_argument("--root",        required=True, help="Project root (folder that contains resources/)")
    s.add_argument("--bin",         required=True, help="Binary output base directory (build/<preset>/bin)")
    s.add_argument("--config",      default=None,  help="Single build config to target (Debug/Release/…)")
    s.add_argument("--all-configs", action="store_true", help="Sync into every config sub-dir that exists")

    args = parser.parse_args()

    if args.command == "build-managed":
        build_managed(
            Path(args.project).resolve(),
            Path(args.output).resolve(),
            Path(args.coral_dir).resolve(),
            args.configuration,
            args.copy_coral,
            args.parallel,
            args.write_props,
        )
    elif args.command == "sync-resources":
        sync_resources(
            Path(args.root).resolve(),
            Path(args.bin).resolve(),
            args.config,
            args.all_configs,
        )


if __name__ == "__main__":
    main()
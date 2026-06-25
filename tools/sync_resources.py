import argparse
import os
import shutil
import subprocess
from pathlib import Path
from typing import Optional

CORAL_ARTIFACTS = ["Coral.Managed.dll", "Coral.Managed.runtimeconfig.json", "Coral.Managed.deps.json", "Coral.Managed.pdb"]
PROPS_TEMPLATE = "<Project>\n  <PropertyGroup>\n    <CoralManagedDir>$(MSBuildThisFileDirectory){relative_coral_dir}</CoralManagedDir>\n  </PropertyGroup>\n</Project>\n"

def find_dotnet() -> Optional[str]:
    if dotnet_root := os.environ.get("DOTNET_ROOT"):
        for name in ("dotnet.exe", "dotnet"):
            if (candidate := Path(dotnet_root) / name).exists():
                return str(candidate)
    
    if dotnet := shutil.which("dotnet"):
        return dotnet

    common = ["C:/Program Files/dotnet/dotnet.exe", "C:/Program Files (x86)/dotnet/dotnet.exe", "/usr/bin/dotnet", "/usr/share/dotnet/dotnet"]
    return next((str(p) for path in common if (p := Path(path)).exists()), None)

def write_if_changed(path: Path, content: str) -> None:
    if not (path.exists() and path.read_text(encoding="utf-8") == content):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

def build_managed(project: Path, output_dir: Path, coral_dir: Path, configuration: str, copy_coral: bool, parallel: bool, write_props: bool) -> None:
    if not (dotnet := find_dotnet()):
        raise FileNotFoundError("Could not find dotnet. Set DOTNET_ROOT or add dotnet to PATH.")

    if write_props:
        relative = os.path.normpath(os.path.relpath(coral_dir, start=project.parent))
        write_if_changed(project.parent / "ManagedDependencies.props", PROPS_TEMPLATE.format(relative_coral_dir=relative))

    output_dir.mkdir(parents=True, exist_ok=True)
    command = [dotnet, "build", str(project), "-c", configuration, "--output", str(output_dir), f"-p:CoralManagedDir={coral_dir}"]
    if parallel:
        command.append("-m")

    subprocess.run(command, cwd=str(project.parent), check=True)

    if copy_coral:
        for name in CORAL_ARTIFACTS:
            if not (src := coral_dir / name).exists():
                raise FileNotFoundError(f"Missing Coral artifact: {src}")
            shutil.copy2(src, output_dir / name)

def sync_resources(project_root: Path, output_dir: Path) -> None:
    src, dst = project_root / "resources", output_dir / "resources"
    if not src.exists():
        print(f"Warning: Source directory '{src}' does not exist.")
        return

    print(f"Syncing engine resources to {dst}...")
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src, dst)
    print("Success: Assets synced!")

def main() -> None:
    parser = argparse.ArgumentParser(description="Build helper entrypoint for Chained Decos")
    subparsers = parser.add_subparsers(dest="command", required=True)

    b_cmd = subparsers.add_parser("build-managed", help="Build a managed C# project")
    b_cmd.add_argument("--project", required=True)
    b_cmd.add_argument("--output", required=True)
    b_cmd.add_argument("--coral-dir", required=True)
    b_cmd.add_argument("--configuration", default="Debug")
    b_cmd.add_argument("--copy-coral", action="store_true")
    b_cmd.add_argument("--write-props", action="store_true")
    b_cmd.add_argument("--parallel", action="store_true")

    s_cmd = subparsers.add_parser("sync-resources", help="Copy engine resources into a binary directory")
    s_cmd.add_argument("--root", required=True)
    s_cmd.add_argument("--bin", required=True)

    args = parser.parse_args()

    if args.command == "build-managed":
        build_managed(Path(args.project).resolve(), Path(args.output).resolve(), Path(args.coral_dir).resolve(), args.configuration, args.copy_coral, args.parallel, args.write_props)
    elif args.command == "sync-resources":
        sync_resources(Path(args.root).resolve(), Path(args.bin).resolve())

if __name__ == "__main__":
    main()
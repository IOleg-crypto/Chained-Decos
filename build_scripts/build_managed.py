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

PROPS_TEMPLATE = "<Project>\n  <PropertyGroup>\n    <CoralManagedDir>$(MSBuildThisFileDirectory){relative_coral_dir}</CoralManagedDir>\n  </PropertyGroup>\n</Project>\n"


def find_dotnet() -> Optional[str]:
    dotnet_root = os.environ.get("DOTNET_ROOT")
    if dotnet_root:
        root_path = Path(dotnet_root)
        for candidate in (root_path / "dotnet.exe", root_path / "dotnet"):
            if candidate.exists():
                return str(candidate)

    dotnet = shutil.which("dotnet")
    if dotnet:
        return dotnet

    common_locations = [
        Path("C:/Program Files/dotnet/dotnet.exe"),
        Path("C:/Program Files (x86)/dotnet/dotnet.exe"),
        Path("/usr/bin/dotnet"),
        Path("/usr/share/dotnet/dotnet"),
    ]
    for candidate in common_locations:
        if candidate.exists():
            return str(candidate)

    return None


def copy_coral_artifacts(coral_dir: Path, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    for artifact_name in CORAL_ARTIFACTS:
        source = coral_dir / artifact_name
        if not source.exists():
            raise FileNotFoundError(f"Missing Coral artifact: {source}")
        shutil.copy2(source, output_dir / artifact_name)


def write_if_changed(path: Path, content: str) -> None:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def write_managed_dependencies_props(project: Path, coral_dir: Path) -> None:
    props_path = project.parent / "ManagedDependencies.props"
    relative_coral_dir = os.path.normpath(os.path.relpath(coral_dir, start=project.parent))
    content = PROPS_TEMPLATE.format(relative_coral_dir=relative_coral_dir)
    write_if_changed(props_path, content)


def sync_resources(project_root: Path, output_dir: Path) -> None:
    resources_src = project_root / "resources"
    resources_dst = output_dir / "resources"

    if not resources_src.exists():
        print(f"Warning: Source directory '{resources_src}' does not exist.")
        return

    print(f"Syncing engine resources to {resources_dst}...")
    if resources_dst.exists():
        shutil.rmtree(resources_dst, ignore_errors=True)
    shutil.copytree(resources_src, resources_dst)
    print("Success: Assets synced!")


def build_managed(project: Path, output_dir: Path, coral_dir: Path, configuration: str, copy_coral: bool, parallel: bool, write_props: bool) -> None:
    dotnet = find_dotnet()
    if not dotnet:
        raise FileNotFoundError("Could not find dotnet. Set DOTNET_ROOT or add dotnet to PATH.")

    if write_props:
        write_managed_dependencies_props(project, coral_dir)

    output_dir.mkdir(parents=True, exist_ok=True)

    command = [
        dotnet,
        "build",
        str(project),
        "-c",
        configuration,
        "--output",
        str(output_dir),
        f"-p:CoralManagedDir={coral_dir}",
    ]
    if parallel:
        command.append("-m")

    subprocess.run(command, cwd=str(project.parent), check=True)

    if copy_coral:
        copy_coral_artifacts(coral_dir, output_dir)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build helper entrypoint for Chained Decos")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_parser = subparsers.add_parser("build-managed", help="Build a managed C# project")
    build_parser.add_argument("--project", required=True, help="Path to the .csproj file")
    build_parser.add_argument("--output", required=True, help="Output directory for the built assembly")
    build_parser.add_argument("--coral-dir", required=True, help="Directory containing Coral.Managed artifacts")
    build_parser.add_argument("--configuration", default="Debug", help="dotnet build configuration")
    build_parser.add_argument("--copy-coral", action="store_true", help="Copy Coral.Managed artifacts into the output directory")
    build_parser.add_argument("--write-props", action="store_true", help="Write ManagedDependencies.props next to the project file")
    build_parser.add_argument("--parallel", action="store_true", help="Enable dotnet parallel build (-m)")

    sync_parser = subparsers.add_parser("sync-resources", help="Copy engine resources into a binary directory")
    sync_parser.add_argument("--root", required=True, help="Project root directory")
    sync_parser.add_argument("--bin", required=True, help="Binary output directory")

    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.command == "build-managed":
        build_managed(
            project=Path(args.project).resolve(),
            output_dir=Path(args.output).resolve(),
            coral_dir=Path(args.coral_dir).resolve(),
            configuration=args.configuration,
            copy_coral=args.copy_coral,
            parallel=args.parallel,
            write_props=args.write_props,
        )
    elif args.command == "sync-resources":
        sync_resources(Path(args.root).resolve(), Path(args.bin).resolve())


if __name__ == "__main__":
    main()
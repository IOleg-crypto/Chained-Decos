"""Build managed C# projects for Chained Engine.

Usage:
    python tools/build_managed.py build-managed \\
        --project managed/Chained.Managed.csproj \\
        --output build/bin/Debug \\
        --coral-dir build/vendor/coral \\
        --configuration Debug \\
        --copy-coral --write-props --parallel
"""
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


def main() -> None:
    parser = argparse.ArgumentParser(description="Build managed C# projects")
    parser.add_argument("--project", required=True, help="Path to .csproj")
    parser.add_argument("--output", required=True, help="Output directory for DLLs")
    parser.add_argument("--coral-dir", required=True, help="Directory containing Coral.Managed artifacts")
    parser.add_argument("--configuration", default="Debug", help="Build configuration (Debug/Release)")
    parser.add_argument("--copy-coral", action="store_true", help="Copy Coral.Managed artifacts to output")
    parser.add_argument("--write-props", action="store_true", help="Write ManagedDependencies.props")
    parser.add_argument("--parallel", action="store_true", help="Use parallel MSBuild")

    args = parser.parse_args()

    build_managed(
        Path(args.project).resolve(),
        Path(args.output).resolve(),
        Path(args.coral_dir).resolve(),
        args.configuration,
        args.copy_coral,
        args.parallel,
        args.write_props,
    )


if __name__ == "__main__":
    main()

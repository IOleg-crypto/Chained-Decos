"""Sync engine resources (shaders, fonts, icons, config) into binary output directories.

Usage:
    python tools/sync_resources.py sync-resources \\
        --root . \\
        --bin build/windows-clang/bin \\
        --all-configs
"""
import argparse
import filecmp
import os
import shutil
from pathlib import Path
from typing import Optional

BUILD_CONFIGS = ["Debug", "Release", "RelWithDebInfo"]


def _sync_dir(src: Path, dst: Path) -> None:
    """Incrementally syncs src into dst — only copies new/changed files."""
    if not src.is_dir():
        return

    src_files = {p.relative_to(src) for p in src.rglob("*") if p.is_file()}
    dst_files = {p.relative_to(dst) for p in dst.rglob("*") if p.is_file()} if dst.is_dir() else set()

    copied = 0
    removed = 0

    for rel in sorted(src_files):
        s = src / rel
        d = dst / rel
        d.parent.mkdir(parents=True, exist_ok=True)
        if not d.exists() or not filecmp.cmp(s, d, shallow=False):
            shutil.copy2(s, d)
            copied += 1

    for rel in sorted(dst_files - src_files):
        (dst / rel).unlink(missing_ok=True)
        removed += 1

    if dst.is_dir():
        for dirpath, _, _ in os.walk(dst, topdown=False):
            try:
                os.rmdir(dirpath)
            except OSError:
                pass

    print(f"  Synced: {src} -> {dst}  ({copied} copied, {removed} removed)")


def sync_resources(
    project_root: Path,
    bin_dir: Path,
    config: Optional[str],
    all_configs: bool,
) -> None:
    """Copy engine resources into the correct binary output folder."""
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
            _sync_dir(resources_src, bin_dir / "resources")
    elif config:
        _sync_dir(resources_src, bin_dir / config / "resources")
    else:
        _sync_dir(resources_src, bin_dir / "resources")

    print("Success: resources synced!")


def main() -> None:
    parser = argparse.ArgumentParser(description="Sync engine resources to build output")
    parser.add_argument("command", nargs="?", help=argparse.SUPPRESS)
    parser.add_argument("--root", required=True, help="Project root (folder that contains resources/)")
    parser.add_argument("--bin", required=True, help="Binary output base directory (build/<preset>/bin)")
    parser.add_argument("--config", default=None, help="Single build config to target (Debug/Release/…)")
    parser.add_argument("--all-configs", action="store_true", help="Sync into every config sub-dir that exists")

    args = parser.parse_args()

    sync_resources(
        Path(args.root).resolve(),
        Path(args.bin).resolve(),
        args.config,
        args.all_configs,
    )


if __name__ == "__main__":
    main()

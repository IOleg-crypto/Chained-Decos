#!/usr/bin/env python3
"""Create resource .pack files for game distribution.

Scans assets/ and resources/ directories, generates file/item path pairs,
and invokes the packer utility to create compressed .pack archives.

Supports splitting into multiple chunks (--split-mb).

Usage:
    python tools/create_pack.py \\
        --packer build/windows-clang/bin/packer.exe \\
        --output  dist/ \\
        --assets   game/chaineddecos/assets \\
        --resources resources \\
        --split-mb 512 \\
        --zip-threshold 5 \\
        --data-version 0
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple


def collect_files(directory: Path, prefix: str = "") -> List[Tuple[Path, str]]:
    """Recursively collect files, returning (absolute_path, virtual_item_path) pairs."""
    items = []
    if not directory.is_dir():
        return items
    for entry in sorted(directory.rglob("*")):
        if not entry.is_file():
            continue
        # Skip build artifacts
        rel = entry.relative_to(directory)
        rel_str = rel.as_posix()
        if any(skip in rel_str for skip in ("/obj/", "/Debug/", "/Release/", "/x64/")):
            continue
        ext = entry.suffix.lower()
        if ext in (".pdb", ".ilk", ".obj", ".tlog", ".log", ".Up2Date", ".cache", ".nupkg"):
            continue
        if entry.stat().st_size == 0:
            continue
        item_path = f"{prefix}/{rel_str}" if prefix else rel_str
        items.append((entry, item_path))
    return items


def split_chunks(
    file_items: List[Tuple[Path, str]], max_chunk_bytes: int
) -> List[List[Tuple[Path, str]]]:
    """Split file list into chunks by uncompressed size."""
    if max_chunk_bytes <= 0:
        return [file_items]
    chunks: List[List[Tuple[Path, str]]] = []
    current: List[Tuple[Path, str]] = []
    current_size = 0
    for fpath, item in file_items:
        fsize = fpath.stat().st_size
        if current and current_size + fsize > max_chunk_bytes:
            chunks.append(current)
            current = []
            current_size = 0
        current.append((fpath, item))
        current_size += fsize
    if current:
        chunks.append(current)
    return chunks


import tempfile


def run_packer(
    packer: str,
    output_path: Path,
    file_items: List[Tuple[Path, str]],
    zip_threshold: int,
    data_version: int,
    prefer_speed: bool,
    use_dictionary: bool = False,
) -> None:
    """Invoke the packer utility with file/item path pairs using manifest file."""
    print(f"  Packer: {output_path.name} ({len(file_items)} files)")

    # Write manifest file (tab-separated)
    with tempfile.NamedTemporaryFile(mode="w", suffix=".txt", delete=False, encoding="utf-8") as mf:
        manifest_path = mf.name
        for fpath, item in file_items:
            mf.write(f"{fpath}\t{item}\n")

    try:
        # Try manifest mode (-m flag) first
        cmd = [packer]
        cmd += ["-z", str(zip_threshold)]
        cmd += ["-v", str(data_version)]
        if prefer_speed:
            cmd += ["-s"]
        if use_dictionary:
            cmd += ["--dict"]
        cmd += ["-m", manifest_path]
        cmd.append(str(output_path))

        result = subprocess.run(cmd, capture_output=True, text=True)

        # If -m flag is unrecognized, the legacy packer.exe doesn't support manifests.
        # chpacker (built from tools/chpacker.cpp) DOES support -m and must be used.
        if result.returncode != 0 and ("unknown option: -m" in result.stderr or "unknown option: -m" in result.stdout):
            print("ERROR: Packer does not support -m (manifest) mode.", file=sys.stderr)
            print("  The legacy packer.exe cannot handle large file lists on Windows.", file=sys.stderr)
            print("  Ensure chpacker.exe is built and used instead:", file=sys.stderr)
            print("  cmake --build <build-dir> --target chpacker", file=sys.stderr)
            sys.exit(1)

        if result.returncode != 0:
            print(f"  ERROR packer failed (rc={result.returncode}):", file=sys.stderr)
            if result.stdout:
                print(result.stdout, file=sys.stderr)
            if result.stderr:
                print(result.stderr, file=sys.stderr)
            sys.exit(1)
        if result.stdout:
            print(f"  {result.stdout.strip()}")
    finally:
        if os.path.exists(manifest_path):
            try:
                os.remove(manifest_path)
            except OSError:
                pass


def main() -> None:
    parser = argparse.ArgumentParser(description="Create resource .pack files for game distribution")
    parser.add_argument("--packer", required=True, help="Path to the packer executable")
    parser.add_argument("--output", required=True, help="Output directory for .pack files")
    parser.add_argument("--assets", help="Path to assets/ directory to pack")
    parser.add_argument("--resources", help="Path to resources/ directory to pack")
    parser.add_argument("--chproject", help="Path to .chproject file to include as project.chproject")
    parser.add_argument("--split-mb", type=int, default=0, help="Split pack into chunks of this size in MB (0 = single file)")
    parser.add_argument("--zip-threshold", type=int, default=5, help="Compression threshold percent (0-100)")
    parser.add_argument("--data-version", type=int, default=0, help="Data version for pack header")
    parser.add_argument("--prefer-speed", action="store_true", help="Use LZ4 instead of ZSTD")
    parser.add_argument("--dict", action="store_true", help="Use ZSTD dictionary compression (best for mixed assets)")
    parser.add_argument("--pack-name", default="resources", help="Base name for pack files (default: resources)")
    args = parser.parse_args()

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Collect all files to pack
    all_items: List[Tuple[Path, str]] = []

    # .chproject → project.chproject
    if args.chproject:
        chproject = Path(args.chproject)
        if chproject.is_file():
            all_items.append((chproject, "project.chproject"))
        else:
            print(f"Warning: .chproject not found: {chproject}", file=sys.stderr)

    # assets/ → assets/...
    if args.assets:
        assets_dir = Path(args.assets)
        asset_items = collect_files(assets_dir, "assets")
        all_items.extend(asset_items)
        print(f"Collected {len(asset_items)} files from {assets_dir}")

    # resources/ → resources/...
    if args.resources:
        res_dir = Path(args.resources)
        res_items = collect_files(res_dir, "resources")
        all_items.extend(res_items)
        print(f"Collected {len(res_items)} files from {res_dir}")

    if not all_items:
        print("No files to pack!", file=sys.stderr)
        sys.exit(1)

    print(f"Total: {len(all_items)} files to pack")

    # Split into chunks
    max_bytes = args.split_mb * 1024 * 1024 if args.split_mb > 0 else 0
    chunks = split_chunks(all_items, max_bytes)
    print(f"Packing into {len(chunks)} chunk(s)...")

    pack_files = []
    for i, chunk in enumerate(chunks):
        if len(chunks) == 1:
            pack_name = f"{args.pack_name}.pack"
        else:
            pack_name = f"{args.pack_name}_{i}.pack"
        pack_path = output_dir / pack_name
        run_packer(
            packer=args.packer,
            output_path=pack_path,
            file_items=chunk,
            zip_threshold=args.zip_threshold,
            data_version=args.data_version,
            prefer_speed=args.prefer_speed,
            use_dictionary=args.dict,
        )
        pack_files.append(pack_path)

    print(f"\nDone! Created {len(pack_files)} pack file(s):")
    for pf in pack_files:
        size_mb = pf.stat().st_size / (1024 * 1024)
        print(f"  {pf.name}  ({size_mb:.1f} MB)")


if __name__ == "__main__":
    main()

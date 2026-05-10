import os
import shutil
import argparse

def sync_assets(project_root, bin_dir):    
    resources_src = os.path.join(project_root, "resources")
    resources_dst = os.path.join(bin_dir, "resources")
    
    if os.path.exists(resources_src):
        print(f"Syncing engine resources to {resources_dst}...")
        if os.path.exists(resources_dst):
            shutil.rmtree(resources_dst, ignore_errors=True)
        shutil.copytree(resources_src, resources_dst)
        print("Success: Assets synced!")
    else:
        print(f"Warning: Source directory '{resources_src}' does not exist.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Sync assets to build directory")
    parser.add_argument("--root", required=True, help="Project root directory")
    parser.add_argument("--bin", required=True, help="Binary output directory")
    args = parser.parse_args()
    
    sync_assets(args.root, args.bin)

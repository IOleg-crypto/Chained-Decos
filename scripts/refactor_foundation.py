import os

root_dirs = ["D:/gitnext/Chained Decos/engine", "D:/gitnext/Chained Decos/editor", "D:/gitnext/Chained Decos/game", "D:/gitnext/Chained Decos/scripting", "D:/gitnext/Chained Decos/runtime"]

replacements = [
    ("engine/core/base.h", "engine/foundation/base.h"),
    ("engine/core/color.h", "engine/foundation/color.h"),
    ("engine/core/engine_assert.h", "engine/foundation/engine_assert.h"),
    ("engine/core/platform_detection.h", "engine/foundation/platform_detection.h"),
    ("engine/core/thread_pool.h", "engine/foundation/thread_pool.h"),
    ("engine/core/timestep.h", "engine/foundation/timestep.h"),
    ("engine/core/uuid.h", "engine/foundation/uuid.h"),
    ("engine/core/zstd_compression.h", "engine/foundation/zstd_compression.h"),
    ("engine/core/yaml_conversions.h", "engine/serialization/yaml_conversions.h")
]

modified_files = 0
for directory in root_dirs:
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".h") or file.endswith(".cpp"):
                path = os.path.join(root, file)
                with open(path, "r", encoding="utf-8") as f:
                    content = f.read()

                new_content = content
                for old_str, new_str in replacements:
                    new_content = new_content.replace(old_str, new_str)

                if new_content != content:
                    with open(path, "w", encoding="utf-8") as f:
                        f.write(new_content)
                    modified_files += 1

print(f"Replaced targets in {modified_files} files.")

import os

root_dirs = ["D:/gitnext/Chained Decos/engine", "D:/gitnext/Chained Decos/editor", "D:/gitnext/Chained Decos/game", "D:/gitnext/Chained Decos/scripting", "D:/gitnext/Chained Decos/runtime"]

replacements = [
    ("engine/core/reflection.h", "engine/reflection/reflection.h"),
    ("engine/core/reflection_rfl.h", "engine/reflection/reflection_rfl.h")
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

print(f"Replaced reflection targets in {modified_files} files.")

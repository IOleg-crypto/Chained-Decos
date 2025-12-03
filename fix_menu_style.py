import re

# Read the file
with open(r'project\chaineddecos\Menu\Menu.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the Initialize method and add SetupStyle() call after resolution sorting
pattern = r'(return a < b;\s+}\);\s+)'
replacement = r'\1\n    // Setup ImGui style now that window and ImGui are initialized\n    SetupStyle();\n'

content = re.sub(pattern, replacement, content)

# Write back
with open(r'project\chaineddecos\Menu\Menu.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Menu.cpp fixed - SetupStyle() call added!")

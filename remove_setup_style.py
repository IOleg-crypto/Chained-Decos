import re

# Read the file  
with open(r'project\chaineddecos\Menu\Menu.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the SetupStyle() call we just added
pattern = r'\n    // Setup ImGui style now that window and ImGui are initialized\n    SetupStyle\(\);\n'
content = re.sub(pattern, '', content)

# Write back
with open(r'project\chaineddecos\Menu\Menu.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Removed SetupStyle() from Menu::Initialize()")

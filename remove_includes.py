import re

# Remove PlayerController include from UIManager.cpp
with open(r'project\chaineddecos\Systems\UIController\UIManager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the include line
content = re.sub(r'#include\s+"\.\.\/PlayerSystem\/PlayerController\.h"\r?\n', '', content)

with open(r'project\chaineddecos\Systems\UIController\UIManager.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("UIManager.cpp - removed PlayerController include")

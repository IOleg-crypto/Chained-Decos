import re

# Fix Player CMakeLists - remove IPlayerMediator.h line
file_path = r'project\chaineddecos\Player\CMakeLists.txt'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the line with IPlayerMediator.h
content = re.sub(r'.*Interfaces/IPlayerMediator\.h.*\r?\n', '', content)

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Removed IPlayerMediator.h from Player/CMakeLists.txt")

# Fix Menu CMakeLists - remove ConsoleManagerHelpers lines
file_path = r'project\chaineddecos\Menu\CMakeLists.txt'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove lines with ConsoleManagerHelpers
content = re.sub(r'.*Console/ConsoleManagerHelpers\.cpp.*\r?\n', '', content)
content = re.sub(r'.*Console/ConsoleManagerHelpers\.h.*\r?\n', '', content)

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Removed ConsoleManagerHelpers from Menu/CMakeLists.txt")

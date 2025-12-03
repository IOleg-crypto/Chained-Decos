import re

file_path = r'project\chaineddecos\GameApplication.cpp'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Remove the UpdateConsoleManagerProviders call and its comment
content = re.sub(r'\s*// Dependency Injection\r?\n\s*UpdateConsoleManagerProviders\(&Engine::Instance\(\)\);\r?\n', '\r\n', content)

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Removed UpdateConsoleManagerProviders call")

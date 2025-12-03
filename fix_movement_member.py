import re

file_path = r'project\chaineddecos\Player\Components\PlayerMovement.h'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Simple replacement
content = content.replace('IPlayerMediator *m_player;', 'IPlayer *m_player;')

with open(file_path, 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(content)

print("Fixed PlayerMovement.h member variable")

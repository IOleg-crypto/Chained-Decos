#include <MapEditor/Application.h>
// # --------
// # int main - init map editor
// # --------
int main() {
    Application editor(1280, 720);
    editor.Init();
    editor.Run();
} 
#include "Window/Window.h"
// # --------
// # int main - init all game
// # --------
int main() {
    Window window(1920 , 1080 , "Chained Decos");
    window.Init();
    window.Run();
}
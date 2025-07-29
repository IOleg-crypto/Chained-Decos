#include "Window/Window.h"
// # --------
// # int main - init all game
// # --------
int main() {
    Window window(1280 , 720 , "Chained Decos");
    window.Init();
    window.Run();
}
#include <Engine/Engine.h>
// # --------
// # int main - init game
// # --------
int main() {
    Engine engine(1920 , 1080);
    engine.Init();
    engine.Run();
}
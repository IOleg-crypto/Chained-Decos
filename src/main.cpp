#include "Engine/Engine.h"
// # --------
// # int main - init game
// # --------
int main() {
    Engine engine(1280 , 720);
    engine.Init();
    engine.Run();
}
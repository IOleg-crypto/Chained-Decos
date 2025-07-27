#include "Window/Window.h"

int main() {
    auto pWindow = std::make_unique<Window>(1920 ,1080, "Chained Decos");
    pWindow->Init();
    pWindow->Run();
}
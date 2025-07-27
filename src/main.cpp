#include "Window/Window.h"

int main() {
    auto pWindow = std::make_unique<Window>(800 ,600, "Chained Decos");
    pWindow->Init();
    pWindow->Run();
}
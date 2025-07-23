#include "Window/Window.h"

int main() {
    auto pWindow = std::make_unique<Window>(800 ,600, "Millennium");
    pWindow->Init();
    pWindow->Run();
}
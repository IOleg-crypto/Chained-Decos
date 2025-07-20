#include "Window/Window.h"

int main() {
    std::unique_ptr<Window> pWindow = std::make_unique<Window>(800 ,600 , "Game");
    pWindow->Init();
    pWindow->Run();
}
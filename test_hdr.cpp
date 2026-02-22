#include "raylib.h"
#include <iostream>

int main() {
    InitWindow(800, 600, "HDR Test");
    Image img = LoadImage("D:/gitnext/Chained Decos/game/chaineddecos/assets/skyboxes/kloppenheim_02_puresky_2k.hdr");
    if (img.data) {
        std::cout << "SUCCESS: format=" << img.format << ", dims=" << img.width << "x" << img.height << "\n";
        Texture2D tex = LoadTextureFromImage(img);
        std::cout << "TEX ID: " << tex.id << "\n";
    } else {
        std::cout << "FAILED to load image\n";
    }
    CloseWindow();
    return 0;
}

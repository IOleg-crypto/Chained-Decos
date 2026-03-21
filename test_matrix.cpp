#include <iostream>
#include "raymath.h"

int main() {
    Matrix s = MatrixScale(2, 2, 2);
    Matrix t = MatrixTranslate(5, 0, 0);
    
    Matrix r1 = MatrixMultiply(s, t);
    Matrix r2 = MatrixMultiply(t, s);
    
    Vector3 v = {1, 0, 0};
    Vector3 out1 = Vector3Transform(v, r1);
    Vector3 out2 = Vector3Transform(v, r2);
    
    std::cout << "S*T (r1): " << out1.x << "\n";
    std::cout << "T*S (r2): " << out2.x << "\n";
    return 0;
}

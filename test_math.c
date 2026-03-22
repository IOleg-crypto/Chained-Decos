#include <stdio.h>

typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Matrix {
    float m0, m4, m8, m12;
    float m1, m5, m9, m13;
    float m2, m6, m10, m14;
    float m3, m7, m11, m15;
} Matrix;

Matrix MatrixTranslate(float x, float y, float z) {
    Matrix result = { 1.0f, 0.0f, 0.0f, x,
                      0.0f, 1.0f, 0.0f, y,
                      0.0f, 0.0f, 1.0f, z,
                      0.0f, 0.0f, 0.0f, 1.0f };
    return result;
}

Matrix MatrixScale(float x, float y, float z) {
    Matrix result = { x, 0.0f, 0.0f, 0.0f,
                      0.0f, y, 0.0f, 0.0f,
                      0.0f, 0.0f, z, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
    return result;
}

Matrix MatrixMultiply(Matrix left, Matrix right) {
    Matrix result = { 0 };
    result.m0 = left.m0*right.m0 + left.m1*right.m4 + left.m2*right.m8 + left.m3*right.m12;
    result.m1 = left.m0*right.m1 + left.m1*right.m5 + left.m2*right.m9 + left.m3*right.m13;
    result.m2 = left.m0*right.m2 + left.m1*right.m6 + left.m2*right.m10 + left.m3*right.m14;
    result.m12 = left.m12*right.m0 + left.m13*right.m4 + left.m14*right.m8 + left.m15*right.m12;
    // ... just need m0 and m12 for this test
    return result;
}

Vector3 Vector3Transform(Vector3 v, Matrix mat) {
    Vector3 result = { 0 };
    result.x = mat.m0*v.x + mat.m4*v.y + mat.m8*v.z + mat.m12;
    return result;
}

int main() {
    Matrix S = MatrixScale(2, 2, 2);
    Matrix T = MatrixTranslate(10, 0, 0);
    
    Matrix ST = MatrixMultiply(S, T);
    Matrix TS = MatrixMultiply(T, S);
    
    Vector3 v = {1, 0, 0}; // Start at X=1
    // If Scale then Translate: X = (1 * 2) + 10 = 12
    // If Translate then Scale: X = (1 + 10) * 2 = 22
    
    Vector3 out_ST = Vector3Transform(v, ST);
    Vector3 out_TS = Vector3Transform(v, TS);
    
    printf("ST X: %f\n", out_ST.x);
    printf("TS X: %f\n", out_TS.x);
    
    return 0;
}

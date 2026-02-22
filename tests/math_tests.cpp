#include "raymath.h"
#include "gtest/gtest.h"

// Basic Math Tests to ensure integration and correctness of Raymath
TEST(MathTest, RaymathVectors)
{
    Vector3 v1 = { 1.0f, 2.0f, 3.0f };
    Vector3 v2 = { 4.0f, 5.0f, 6.0f };
    
    Vector3 added = Vector3Add(v1, v2);
    EXPECT_FLOAT_EQ(added.x, 5.0f);
    EXPECT_FLOAT_EQ(added.y, 7.0f);
    EXPECT_FLOAT_EQ(added.z, 9.0f);

    float dot = Vector3DotProduct(v1, v2);
    EXPECT_FLOAT_EQ(dot, 32.0f);
}

TEST(MathTest, RaymathMatrices)
{
    Matrix identity = MatrixIdentity();
    EXPECT_FLOAT_EQ(identity.m0, 1.0f);
    EXPECT_FLOAT_EQ(identity.m5, 1.0f);
    EXPECT_FLOAT_EQ(identity.m10, 1.0f);
    EXPECT_FLOAT_EQ(identity.m15, 1.0f);

    Matrix trans = MatrixTranslate(10.0f, 20.0f, 30.0f);
    EXPECT_FLOAT_EQ(trans.m12, 10.0f);
    EXPECT_FLOAT_EQ(trans.m13, 20.0f);
    EXPECT_FLOAT_EQ(trans.m14, 30.0f);
}

TEST(MathTest, NegativeDivisionByZeroFallback)
{
    // Test how raymath handles normalization of zero vector
    Vector3 zero = { 0.0f, 0.0f, 0.0f };
    Vector3 norm = Vector3Normalize(zero);
    
    // Raymath fallback for zero vector normalization should ideally be zero or valid floats.
    // It returns zero vector.
    EXPECT_FLOAT_EQ(norm.x, 0.0f);
    EXPECT_FLOAT_EQ(norm.y, 0.0f);
    EXPECT_FLOAT_EQ(norm.z, 0.0f);
}

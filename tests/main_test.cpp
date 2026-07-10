// main_test.cpp
// Minimal test entry — just gtest, no engine.
#include "gtest/gtest.h"

TEST(MinimalTest, SanityCheck)
{
    EXPECT_EQ(1 + 1, 2);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include "engine/physics/bvh/bvh.h"
#include "gtest/gtest.h"

using namespace Chained;

TEST(BVHTest, SimpleTriangleRaycast)
{
    std::vector<CollisionTriangle> tris;
    // Triangle in XY plane at Z=5
    tris.emplace_back(glm::vec3(-1, -1, 5), glm::vec3(1, -1, 5), glm::vec3(0, 1, 5), 0);

    auto bvh = BVH::Build(std::move(tris));
    ASSERT_NE(bvh, nullptr);

    Ray ray;
    ray.position = {0, 0, 0};
    ray.direction = {0, 0, 1};

    float t = 100.0f;
    glm::vec3 normal;
    int meshIndex;
    EXPECT_TRUE(bvh->Raycast(ray, t, normal, meshIndex));
    EXPECT_NEAR(t, 5.0f, 0.001f);
    EXPECT_EQ(meshIndex, 0);

    // Ray missing triangle
    ray.position = {2, 0, 0};
    t = 100.0f;
    EXPECT_FALSE(bvh->Raycast(ray, t, normal, meshIndex));
}

TEST(BVHTest, MultipleTrianglesQuery)
{
    std::vector<CollisionTriangle> tris;
    tris.emplace_back(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), 101);
    tris.emplace_back(glm::vec3(10, 0, 0), glm::vec3(11, 0, 0), glm::vec3(10, 1, 0), 102);

    auto bvh = BVH::Build(std::move(tris));
    ASSERT_NE(bvh, nullptr);

    BoundingBox box = {{ -1, -1, -1 }, { 2, 2, 2 }};
    std::vector<const CollisionTriangle*> results;
    bvh->QueryAABB(box, results);
    EXPECT_EQ(results.size(), 1);
    if (!results.empty()) EXPECT_EQ(results[0]->meshIndex, 101);

    results.clear();
    box = {{ 9, -1, -1 }, { 12, 2, 2 }};
    bvh->QueryAABB(box, results);
    EXPECT_EQ(results.size(), 1);
    if (!results.empty()) EXPECT_EQ(results[0]->meshIndex, 102);
}

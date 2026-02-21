#include "engine/core/uuid.h"
#include "gtest/gtest.h"
#include <unordered_set>

using namespace CHEngine;

TEST(UUIDTest, GenerationUniqueness)
{
    std::unordered_set<uint64_t> uuids;
    const int count = 1000;
    for (int i = 0; i < count; i++)
    {
        UUID id;
        uuids.insert((uint64_t)id);
    }

    EXPECT_EQ(uuids.size(), count);
}

TEST(UUIDTest, CopyAndAssignment)
{
    UUID id1;
    UUID id2 = id1;
    EXPECT_EQ((uint64_t)id1, (uint64_t)id2);

    UUID id3;
    id3 = id1;
    EXPECT_EQ((uint64_t)id1, (uint64_t)id3);
}

TEST(UUIDTest, StringConversionPlaceholder)
{
    // If we add string conversion in future
    UUID id(12345);
    EXPECT_EQ((uint64_t)id, 12345);
}

TEST(UUIDTest, Hashability)
{
    std::unordered_map<UUID, std::string> uuidMap;
    UUID id1, id2;

    uuidMap[id1] = "ID1";
    uuidMap[id2] = "ID2";

    EXPECT_EQ(uuidMap[id1], "ID1");
    EXPECT_EQ(uuidMap[id2], "ID2");
    EXPECT_EQ(uuidMap.size(), 2);
}

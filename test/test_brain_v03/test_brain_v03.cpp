#include <unity.h>
#include <cstring>
#include <string>
#include "vision_grid_protocol.h"
#include "visual_memory.h"

void test_grid_packet_parser() {
    std::string s = "G,123,4,90,50,";
    static const char h[] = "0123456789ABCDEF";
    for (int i = 0; i < WALLZ_SNAPSHOT_GRID_CELLS; ++i) {
        const uint8_t v = static_cast<uint8_t>(i);
        s.push_back(h[v >> 4]);
        s.push_back(h[v & 15]);
    }
    VisionGridSnapshot g;
    TEST_ASSERT_TRUE(parseVisionGridSnapshot(s.c_str(), g));
    TEST_ASSERT_EQUAL_UINT32(123, g.ms);
    TEST_ASSERT_EQUAL_UINT16(4, g.seq);
    TEST_ASSERT_EQUAL_UINT8(255, g.grid[255]);
}

void fillVertical(VisionGridSnapshot& g, int shift = 0) {
    memset(g.grid, 35, sizeof(g.grid));
    for (int y = 3; y <= 12; ++y) {
        for (int x = 5 + shift; x <= 12 + shift; ++x) {
            if (x >= 0 && x < WALLZ_SNAPSHOT_GRID_W)
                g.grid[y * WALLZ_SNAPSHOT_GRID_W + x] = 220;
        }
    }
}

void test_visual_teach_recognize_reward() {
    VisualMemory m;
    VisionGridSnapshot g{};
    fillVertical(g, 0);
    m.observe(g);
    TEST_ASSERT_TRUE(m.teach("person"));
    TEST_ASSERT_EQUAL_INT(1, m.conceptCount());

    VisionGridSnapshot shifted{};
    fillVertical(shifted, 1);
    const auto r = m.observe(shifted);
    TEST_ASSERT_TRUE(r.index >= 0);
    TEST_ASSERT_EQUAL_STRING("person", r.label);
    TEST_ASSERT_TRUE(r.score >= WALLZ_VISUAL_RECOGNIZE_THRESHOLD);

    m.rewardCurrent(1.0f);
    TEST_ASSERT_TRUE(m.current().value_milli > 0);

    const VisualMemoryPersist p = m.persist();
    VisualMemory restored;
    TEST_ASSERT_TRUE(restored.restore(p));
    TEST_ASSERT_EQUAL_INT(1, restored.conceptCount());
}

void test_invalid_label_rejected() {
    VisualMemory m;
    VisionGridSnapshot g{};
    fillVertical(g);
    m.observe(g);
    TEST_ASSERT_FALSE(m.teach("not a valid label"));
}

void setUp() {}
void tearDown() {}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_grid_packet_parser);
    RUN_TEST(test_visual_teach_recognize_reward);
    RUN_TEST(test_invalid_label_rejected);
    return UNITY_END();
}

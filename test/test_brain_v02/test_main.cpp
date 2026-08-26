#include <unity.h>
#include <cmath>
#include "brain_protocol.h"
#include "brain_core.h"
#include "vision_protocol.h"
#include "vision_grid.h"
#include <cstring>

void setUp(void) {}
void tearDown(void) {}

void test_protocol_parse() {
    BrainTelemetry t;
    TEST_ASSERT_TRUE(parseBrainTelemetry("T,100,500,1000,900,100,110,1,2,3,90,135,0,0,1", t));
    TEST_ASSERT_EQUAL_UINT32(100, t.ms);
    TEST_ASSERT_EQUAL_INT(500, t.distance_mm);
    TEST_ASSERT_EQUAL_INT(135, t.head_z);
    TEST_ASSERT_EQUAL_INT(1, t.brain_armed);
    TEST_ASSERT_EQUAL_INT(0, t.vision_online);
    TEST_ASSERT_FALSE(parseBrainTelemetry("X,100", t));
}

void test_vision_protocol_parse() {
    VisionTelemetry v;
    TEST_ASSERT_TRUE(parseVisionTelemetry("V,200,81,-420,110,127,44,198,7", v));
    TEST_ASSERT_EQUAL_UINT32(200, v.ms);
    TEST_ASSERT_EQUAL_INT(81, v.motion);
    TEST_ASSERT_EQUAL_INT(-420, v.x);
    TEST_ASSERT_EQUAL_INT(198, v.fps_x10);
    TEST_ASSERT_EQUAL_UINT32(7, v.flags);
}


void test_vision_grid_left_motion() {
    uint8_t prev[WALLZ_VISION_GRID_CELLS] = {};
    bool have = false;
    static uint8_t f0[WALLZ_VISION_FRAME_W * WALLZ_VISION_FRAME_H];
    static uint8_t f1[WALLZ_VISION_FRAME_W * WALLZ_VISION_FRAME_H];
    memset(f0, 80, sizeof(f0));
    memset(f1, 80, sizeof(f1));
    wallzAnalyzeGrayFrame(f0, WALLZ_VISION_FRAME_W, WALLZ_VISION_FRAME_H, prev, have, 18);
    for (int y=30; y<90; ++y) for (int x=8; x<56; ++x) f1[y*WALLZ_VISION_FRAME_W+x]=220;
    const WallZVisionResult r = wallzAnalyzeGrayFrame(f1, WALLZ_VISION_FRAME_W, WALLZ_VISION_FRAME_H, prev, have, 18);
    TEST_ASSERT_TRUE(r.motion > 45);
    TEST_ASSERT_TRUE(r.x < -150);
}

void test_obstacle_is_stop() {
    BrainCore b;
    BrainTelemetry t;
    t.distance_mm = 250;
    b.observe(t);
    TEST_ASSERT_EQUAL_INT((int)BrainContext::Obstacle, (int)b.classify(t));
    TEST_ASSERT_EQUAL_INT((int)BrainAction::Stop, (int)b.suggest(t));
}

void test_fisheye_motion_steers_attention() {
    BrainCore b;
    BrainTelemetry t;
    t.distance_mm = 1000;
    t.vision_online = 1;
    t.vision_motion = 100;
    t.vision_x = -600;
    b.observe(t);
    TEST_ASSERT_EQUAL_INT((int)BrainContext::MotionEvent, (int)b.classify(t));
    TEST_ASSERT_EQUAL_INT((int)BrainAction::LookLeft, (int)b.suggest(t));
    t.vision_x = 650;
    TEST_ASSERT_EQUAL_INT((int)BrainAction::LookRight, (int)b.suggest(t));
}

void test_reward_increases_q_value() {
    BrainCore b;
    BrainTelemetry t;
    t.distance_mm = 1000;
    t.light_l = 2200;
    t.light_r = 300;
    b.observe(t);
    b.markAction(BrainContext::LightLeft, BrainAction::LookLeft);
    const float before = b.q(BrainContext::LightLeft, BrainAction::LookLeft);
    b.reward(1.0f, BrainContext::Calm);
    const float after = b.q(BrainContext::LightLeft, BrainAction::LookLeft);
    TEST_ASSERT_TRUE(after > before);
}

void test_persist_restore() {
    BrainCore b;
    b.markAction(BrainContext::Calm, BrainAction::CreepForward);
    b.reward(0.8f, BrainContext::Calm);
    const BrainPersist p = b.persist();
    BrainCore c;
    TEST_ASSERT_TRUE(c.restore(p));
    TEST_ASSERT_FLOAT_WITHIN(1e-6f,
        b.q(BrainContext::Calm, BrainAction::CreepForward),
        c.q(BrainContext::Calm, BrainAction::CreepForward));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_protocol_parse);
    RUN_TEST(test_vision_protocol_parse);
    RUN_TEST(test_vision_grid_left_motion);
    RUN_TEST(test_obstacle_is_stop);
    RUN_TEST(test_fisheye_motion_steers_attention);
    RUN_TEST(test_reward_increases_q_value);
    RUN_TEST(test_persist_restore);
    return UNITY_END();
}

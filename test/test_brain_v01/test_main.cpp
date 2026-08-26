#include <unity.h>
#include <cmath>
#include "brain_protocol.h"
#include "brain_core.h"

void setUp(void) {}
void tearDown(void) {}

void test_protocol_parse() {
    BrainTelemetry t;
    TEST_ASSERT_TRUE(parseBrainTelemetry("T,100,500,1000,900,100,110,1,2,3,90,135,0,0,1", t));
    TEST_ASSERT_EQUAL_UINT32(100, t.ms);
    TEST_ASSERT_EQUAL_INT(500, t.distance_mm);
    TEST_ASSERT_EQUAL_INT(135, t.head_z);
    TEST_ASSERT_EQUAL_INT(1, t.brain_armed);
    TEST_ASSERT_FALSE(parseBrainTelemetry("X,100", t));
}

void test_obstacle_is_stop() {
    BrainCore b;
    BrainTelemetry t;
    t.distance_mm = 250;
    b.observe(t);
    TEST_ASSERT_EQUAL_INT((int)BrainContext::Obstacle, (int)b.classify(t));
    TEST_ASSERT_EQUAL_INT((int)BrainAction::Stop, (int)b.suggest(t));
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

void test_forward_open_space_is_safe() {
    TEST_ASSERT_TRUE(brainForwardIsSafe(-1));
    TEST_ASSERT_TRUE(brainForwardIsSafe(5000));
    TEST_ASSERT_TRUE(brainForwardIsSafe(350));
    TEST_ASSERT_FALSE(brainForwardIsSafe(349));
    TEST_ASSERT_FALSE(brainForwardIsSafe(200));
    TEST_ASSERT_EQUAL(500, BRAIN_SONAR_MAX_CM);
    TEST_ASSERT_EQUAL_UINT32(29000u, BRAIN_SONAR_TIMEOUT_US);
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
    RUN_TEST(test_obstacle_is_stop);
    RUN_TEST(test_reward_increases_q_value);
    RUN_TEST(test_forward_open_space_is_safe);
    RUN_TEST(test_persist_restore);
    return UNITY_END();
}

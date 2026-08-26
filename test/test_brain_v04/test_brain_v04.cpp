#include <unity.h>
#include <cstring>

#include "brain_protocol.h"
#include "manual_demo.h"
#include "imitation_memory.h"

void setUp() {}
void tearDown() {}

static BrainTelemetry baseTelemetry() {
    BrainTelemetry t;
    t.distance_mm = 1200;
    t.light_l = 700;
    t.light_r = 650;
    t.mic_l = 100;
    t.mic_r = 110;
    t.head_xy = 90;
    t.head_z = 135;
    t.vision_online = 1;
    t.vision_motion = 80;
    t.vision_x = -200;
    t.vision_familiarity = 850;
    return t;
}

void test_manual_demo_parser() {
    ManualDemonstration d;
    TEST_ASSERT_TRUE(parseManualDemonstration("D,1234,-25,91,0,0,1,0", d));
    TEST_ASSERT_EQUAL_UINT32(1234, d.ms);
    TEST_ASSERT_EQUAL_INT(-25, d.lx);
    TEST_ASSERT_EQUAL_INT(91, d.ly);
    TEST_ASSERT_EQUAL_INT(1, d.drive_active);
    TEST_ASSERT_FALSE(parseManualDemonstration("X,1,2", d));
}

void test_manual_action_mapping() {
    ManualDemonstration d;
    d.drive_active = 1;
    d.ly = 90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::Forward, (int)ImitationMemory::actionFromManual(d));
    d.ly = -90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::Backward, (int)ImitationMemory::actionFromManual(d));
    d.ly = 0; d.lx = 90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::TurnRight, (int)ImitationMemory::actionFromManual(d));
    d.lx = -90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::TurnLeft, (int)ImitationMemory::actionFromManual(d));

    d = ManualDemonstration{};
    d.head_active = 1; d.rx = -90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::LookLeft, (int)ImitationMemory::actionFromManual(d));
    d.rx = 90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::LookRight, (int)ImitationMemory::actionFromManual(d));
    d.rx = 0; d.ry = -90;
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::LookUp, (int)ImitationMemory::actionFromManual(d));
}

void test_imitation_learn_predict_persist() {
    ImitationMemory m;
    ManualDemonstration d;
    d.drive_active = 1;
    d.ly = 100;

    BrainTelemetry a = baseTelemetry();
    BrainTelemetry b = baseTelemetry(); b.distance_mm = 2200; b.vision_x = -550;
    BrainTelemetry c = baseTelemetry(); c.distance_mm = 3600; c.light_l = 1300; c.light_r = 500;

    TEST_ASSERT_TRUE(m.learn(d, a, ImitationMemory::tagFromLabel("person")));
    TEST_ASSERT_TRUE(m.learn(d, b, ImitationMemory::tagFromLabel("person")));
    TEST_ASSERT_TRUE(m.learn(d, c, ImitationMemory::tagFromLabel("person")));
    TEST_ASSERT_EQUAL_INT(3, m.count());

    BrainTelemetry q = baseTelemetry();
    q.distance_mm = 1350;
    const ImitationPrediction p = m.predict(q, ImitationMemory::tagFromLabel("person"));
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::Forward, (int)p.action);
    TEST_ASSERT_TRUE(p.confidence >= WALLZ_IMITATION_EXEC_THRESHOLD);

    const ImitationPersist saved = m.persist();
    ImitationMemory restored;
    TEST_ASSERT_TRUE(restored.restore(saved));
    TEST_ASSERT_EQUAL_INT(3, restored.count());
    const ImitationPrediction p2 = restored.predict(q, ImitationMemory::tagFromLabel("person"));
    TEST_ASSERT_EQUAL_INT((int)ImitationAction::Forward, (int)p2.action);
}

void test_duplicate_is_rejected() {
    ImitationMemory m;
    ManualDemonstration d; d.drive_active = 1; d.ly = 100;
    BrainTelemetry t = baseTelemetry();
    TEST_ASSERT_TRUE(m.learn(d, t, 0));
    TEST_ASSERT_FALSE(m.learn(d, t, 0));
    TEST_ASSERT_EQUAL_INT(1, m.count());
    TEST_ASSERT_EQUAL_UINT32(1, m.rejectedDuplicate());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_manual_demo_parser);
    RUN_TEST(test_manual_action_mapping);
    RUN_TEST(test_imitation_learn_predict_persist);
    RUN_TEST(test_duplicate_is_rejected);
    return UNITY_END();
}

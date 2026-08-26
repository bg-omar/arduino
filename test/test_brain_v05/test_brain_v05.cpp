#include <unity.h>
#include "wallz_camera_storage_protocol.h"
#include "camera_storage_policy.h"

void test_context_protocol() {
    CameraBrainContext c;
    TEST_ASSERT_TRUE(wallzParseBrainContext("C,CTX,person,912,88,250", c));
    TEST_ASSERT_EQUAL_STRING("person", c.label);
    TEST_ASSERT_EQUAL(912, c.familiarity);
    TEST_ASSERT_EQUAL(88, c.novelty);
    TEST_ASSERT_EQUAL(250, c.value_milli);
    TEST_ASSERT_FALSE(wallzParseBrainContext("C,CTX,bad label,900,100,0", c));
}

void test_store_protocol() {
    CameraStoreRequest r;
    TEST_ASSERT_TRUE(wallzParseStoreRequest("C,SAVE,teach,Kato", r));
    TEST_ASSERT_EQUAL_STRING("teach", r.reason);
    TEST_ASSERT_EQUAL_STRING("Kato", r.label);
}

void test_storage_status_protocol() {
    CameraStorageStatus s;
    TEST_ASSERT_TRUE(wallzParseStorageStatus("VS,SD,1,29684,412,17,19,0", s));
    TEST_ASSERT_TRUE(s.valid);
    TEST_ASSERT_TRUE(s.mounted);
    TEST_ASSERT_EQUAL_UINT32(29684, s.total_mb);
    TEST_ASSERT_EQUAL_UINT32(17, s.frames);
    TEST_ASSERT_EQUAL_UINT32(19, s.events);
}

void test_local_first_policy() {
    CameraStoragePolicy p;
    CameraStoragePolicyInput in;
    in.now_ms = 6000;
    in.motion = 300;
    in.brain_context_fresh = false;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CameraAutoSaveReason::MotionOffline), static_cast<int>(p.evaluate(in)));

    in.now_ms = 12000;
    in.brain_context_fresh = true;
    in.motion = 120;
    in.familiarity = 500;
    in.novelty = 200;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CameraAutoSaveReason::UnknownMotion), static_cast<int>(p.evaluate(in)));

    in.now_ms = 18000;
    in.motion = 120;
    in.familiarity = 900;
    in.novelty = 100;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CameraAutoSaveReason::None), static_cast<int>(p.evaluate(in)));
}

void test_explicit_save_cooldown() {
    CameraStoragePolicy p;
    TEST_ASSERT_TRUE(p.acceptExplicit(1000));
    TEST_ASSERT_FALSE(p.acceptExplicit(1100));
    TEST_ASSERT_TRUE(p.acceptExplicit(1400));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_context_protocol);
    RUN_TEST(test_store_protocol);
    RUN_TEST(test_storage_status_protocol);
    RUN_TEST(test_local_first_policy);
    RUN_TEST(test_explicit_save_cooldown);
    return UNITY_END();
}

#include <unity.h>
#include <cstring>
#include "wallz_episode_protocol.h"
#include "episode_capture_policy.h"

void test_episode_request_protocol() {
    EpisodeRequest req;
    TEST_ASSERT_TRUE(wallzParseEpisodeRequest("C,EP,reward_pos,person,2000,3000", req));
    TEST_ASSERT_EQUAL_STRING("reward_pos", req.reason);
    TEST_ASSERT_EQUAL_STRING("person", req.label);
    TEST_ASSERT_EQUAL(2000, req.pre_ms);
    TEST_ASSERT_EQUAL(3000, req.post_ms);
    TEST_ASSERT_FALSE(wallzParseEpisodeRequest("C,EP,bad reason,person,2000,3000", req));
}

void test_episode_event_protocol() {
    EpisodeEvent ev;
    TEST_ASSERT_TRUE(wallzParseEpisodeEvent("VE,BEGIN,4,12,teach,ball,10,15", ev));
    TEST_ASSERT_EQUAL(static_cast<int>(EpisodeEvent::Type::Begin), static_cast<int>(ev.type));
    TEST_ASSERT_EQUAL(4, ev.session);
    TEST_ASSERT_EQUAL(12, ev.episode);
    TEST_ASSERT_EQUAL(10, ev.pre_frames);
    TEST_ASSERT_EQUAL(15, ev.post_frames);
    TEST_ASSERT_TRUE(wallzParseEpisodeEvent("VE,DONE,4,12,teach,ball,26,0", ev));
    TEST_ASSERT_EQUAL(static_cast<int>(EpisodeEvent::Type::Done), static_cast<int>(ev.type));
    TEST_ASSERT_EQUAL(26, ev.total_frames);
    TEST_ASSERT_EQUAL(0, ev.errors);
}

void test_episode_capture_timing_policy() {
    EpisodeCapturePolicy p;
    TEST_ASSERT_TRUE(p.sampleDue(100));
    TEST_ASSERT_FALSE(p.sampleDue(299));
    TEST_ASSERT_TRUE(p.sampleDue(300));
    TEST_ASSERT_TRUE(p.canTrigger(300));
    p.begin(300, 3000);
    TEST_ASSERT_TRUE(p.active());
    TEST_ASSERT_FALSE(p.postComplete(3299));
    TEST_ASSERT_TRUE(p.postComplete(3300));
    p.notePostFrame();
    TEST_ASSERT_EQUAL(1, p.postFrames());
    p.finish();
    TEST_ASSERT_FALSE(p.active());
    TEST_ASSERT_FALSE(p.canTrigger(1000));
    TEST_ASSERT_TRUE(p.canTrigger(1500));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_episode_request_protocol);
    RUN_TEST(test_episode_event_protocol);
    RUN_TEST(test_episode_capture_timing_policy);
    return UNITY_END();
}

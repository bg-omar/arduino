#include <unity.h>
#include "sense_playful.h"

void test_sense_playful_pick_cycles() {
	TEST_ASSERT_EQUAL(static_cast<int>(SensePlayfulMode::Dance),
		static_cast<int>(sensePlayfulPick(0)));
	TEST_ASSERT_EQUAL(static_cast<int>(SensePlayfulMode::Avoid),
		static_cast<int>(sensePlayfulPick(1)));
	TEST_ASSERT_EQUAL(static_cast<int>(SensePlayfulMode::FollowLight),
		static_cast<int>(sensePlayfulPick(2)));
	TEST_ASSERT_EQUAL(static_cast<int>(SensePlayfulMode::Dance),
		static_cast<int>(sensePlayfulPick(3)));
}

void test_sense_playful_should_start() {
	TEST_ASSERT_FALSE(sensePlayfulShouldStart(11999, false, false, SensePlayfulMode::None, 20000, 0));
	TEST_ASSERT_TRUE(sensePlayfulShouldStart(12000, false, false, SensePlayfulMode::None, 20000, 0));
	TEST_ASSERT_FALSE(sensePlayfulShouldStart(20000, true, false, SensePlayfulMode::None, 30000, 0));
	TEST_ASSERT_FALSE(sensePlayfulShouldStart(20000, false, true, SensePlayfulMode::None, 30000, 0));
	TEST_ASSERT_FALSE(sensePlayfulShouldStart(20000, false, false, SensePlayfulMode::Dance, 30000, 0));
	TEST_ASSERT_FALSE(sensePlayfulShouldStart(20000, false, false, SensePlayfulMode::None, 5000, 10000));
}

void test_sense_playful_burst_expired() {
	TEST_ASSERT_FALSE(sensePlayfulBurstExpired(1000, 0));
	TEST_ASSERT_FALSE(sensePlayfulBurstExpired(11999, 1000));
	TEST_ASSERT_TRUE(sensePlayfulBurstExpired(13000, 1000));
	TEST_ASSERT_FALSE(sensePlayfulBurstExpired(21999, 10000));
	TEST_ASSERT_TRUE(sensePlayfulBurstExpired(22000, 10000));
}

void test_sense_playful_pick_idle_always_avoid() {
	TEST_ASSERT_EQUAL(static_cast<int>(SensePlayfulMode::Avoid),
		static_cast<int>(sensePlayfulPickIdle()));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_sense_playful_pick_cycles);
	RUN_TEST(test_sense_playful_should_start);
	RUN_TEST(test_sense_playful_burst_expired);
	RUN_TEST(test_sense_playful_pick_idle_always_avoid);
	return UNITY_END();
}

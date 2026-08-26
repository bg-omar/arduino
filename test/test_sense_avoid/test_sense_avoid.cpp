#include <unity.h>
#include "sense_avoid.h"

void test_sense_avoid_clear_when_far() {
	SenseAvoidState st{};
	const SenseAvoidOutput out = senseAvoidStep(st, 40.0f, -1.0f, -1.0f, 0);
	TEST_ASSERT_FALSE(out.overrideMotor);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseAvoidPhase::Clear), static_cast<int>(out.phase));
}

void test_sense_avoid_blocks_when_close() {
	SenseAvoidState st{};
	const SenseAvoidOutput out = senseAvoidStep(st, 20.0f, -1.0f, -1.0f, 0);
	TEST_ASSERT_TRUE(out.overrideMotor);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseMotorIntent::Stop), static_cast<int>(out.motor));
}

void test_sense_avoid_pick_turn_left_when_left_clearer() {
	TEST_ASSERT_TRUE(senseAvoidPickTurnLeft(60.0f, 20.0f));
	TEST_ASSERT_FALSE(senseAvoidPickTurnLeft(20.0f, 60.0f));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_sense_avoid_clear_when_far);
	RUN_TEST(test_sense_avoid_blocks_when_close);
	RUN_TEST(test_sense_avoid_pick_turn_left_when_left_clearer);
	return UNITY_END();
}

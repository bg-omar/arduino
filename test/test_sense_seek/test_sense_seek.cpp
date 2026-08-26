#include <unity.h>
#include "sense_seek.h"
#include "sense_thresholds.h"

void test_seek_inactive_before_timeout() {
	TEST_ASSERT_FALSE(senseSeekActive(3999));
}

void test_seek_active_after_timeout() {
	TEST_ASSERT_TRUE(senseSeekActive(4000));
}

void test_threshold_ramp_to_sixty_percent() {
	const SenseThresholds t = senseEffectiveThresholds(SENSE_SEEK_AFTER_MS + SENSE_SEEK_RAMP_MS);
	TEST_ASSERT_EQUAL_INT(48, t.micLoud);
	TEST_ASSERT_EQUAL_INT(390, t.lightThresh);
}

void test_seek_pan_sequence() {
	SenseSeekState st{};
	const SenseSeekOutput first = senseSeekStep(st, 0, 5000, 0, 0);
	TEST_ASSERT_TRUE(first.active);
	TEST_ASSERT_EQUAL_INT(SENSE_SEEK_HEAD_CENTER, first.headXY);

	const SenseSeekOutput left = senseSeekStep(st, SENSE_SEEK_STEP_MS + 1, 5000, 0, 0);
	TEST_ASSERT_EQUAL_INT(SENSE_SEEK_HEAD_LEFT, left.headXY);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_seek_inactive_before_timeout);
	RUN_TEST(test_seek_active_after_timeout);
	RUN_TEST(test_threshold_ramp_to_sixty_percent);
	RUN_TEST(test_seek_pan_sequence);
	return UNITY_END();
}

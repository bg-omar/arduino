#include <unity.h>
#include "ps4_head.h"

void test_right_stick_decode() {
	int rx = 0;
	int ry = 0;
	TEST_ASSERT_TRUE(ps4UpdateRightStick(8127 + 40, 9127 - 30, rx, ry));
	TEST_ASSERT_EQUAL_INT(40, rx);
	TEST_ASSERT_EQUAL_INT(-30, ry);
}

void test_right_stick_single_axis_keeps_other() {
	int rx = 50;
	int ry = -40;
	TEST_ASSERT_TRUE(ps4UpdateRightStick(8127, 8127, rx, ry));
	TEST_ASSERT_EQUAL_INT(0, rx);
	TEST_ASSERT_EQUAL_INT(-40, ry);
}

void test_head_deadzone_inactive() {
	TEST_ASSERT_FALSE(ps4HeadStickActive(0, 0));
	TEST_ASSERT_FALSE(ps4HeadStickActive(19, -19));
	TEST_ASSERT_TRUE(ps4HeadStickActive(20, 0));
	TEST_ASSERT_TRUE(ps4HeadStickActive(0, -20));
}

void test_head_targets_absolute_and_clamped() {
	TEST_ASSERT_EQUAL_INT(PS4_HEAD_XY_MAX, ps4HeadTargetXY(-128));
	TEST_ASSERT_EQUAL_INT(PS4_HEAD_XY_MIN, ps4HeadTargetXY(127));
	// Y inverted: stick up (-) → Z_MIN, stick down (+) → Z_MAX
	TEST_ASSERT_EQUAL_INT(PS4_HEAD_Z_MIN, ps4HeadTargetZ(-128));
	TEST_ASSERT_EQUAL_INT(PS4_HEAD_Z_MAX, ps4HeadTargetZ(127));

	const Ps4HeadTargets mid = ps4HeadTargetsFromStick(0, 0);
	TEST_ASSERT_FALSE(mid.active);

	const Ps4HeadTargets left = ps4HeadTargetsFromStick(-80, 0);
	TEST_ASSERT_TRUE(left.active);
	TEST_ASSERT_TRUE(left.xy > 90);
}

void test_head_slew_limits_jump() {
	TEST_ASSERT_EQUAL_INT(100, ps4HeadSlew(90, 150, 10));
	TEST_ASSERT_EQUAL_INT(80, ps4HeadSlew(90, 50, 10));
	TEST_ASSERT_EQUAL_INT(95, ps4HeadSlew(90, 95, 10));
}

void test_head_max_delta_from_dt() {
	TEST_ASSERT_EQUAL_INT(0, ps4HeadMaxDelta(0, 120));
	TEST_ASSERT_EQUAL_INT(1, ps4HeadMaxDelta(1, 120));
	TEST_ASSERT_EQUAL_INT(6, ps4HeadMaxDelta(50, 120));
	// Cap large gaps so a stalled loop cannot teleport the head.
	TEST_ASSERT_EQUAL_INT(6, ps4HeadMaxDelta(500, 120));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_right_stick_decode);
	RUN_TEST(test_right_stick_single_axis_keeps_other);
	RUN_TEST(test_head_deadzone_inactive);
	RUN_TEST(test_head_targets_absolute_and_clamped);
	RUN_TEST(test_head_slew_limits_jump);
	RUN_TEST(test_head_max_delta_from_dt);
	return UNITY_END();
}

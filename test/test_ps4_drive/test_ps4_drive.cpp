#include <unity.h>
#include "ps4_drive.h"

void test_deadzone_zeros_center_noise() {
	TEST_ASSERT_EQUAL_INT(0, ps4ApplyDeadzone(0));
	TEST_ASSERT_EQUAL_INT(0, ps4ApplyDeadzone(19));
	TEST_ASSERT_EQUAL_INT(0, ps4ApplyDeadzone(-19));
	TEST_ASSERT_EQUAL_INT(20, ps4ApplyDeadzone(20));
	TEST_ASSERT_EQUAL_INT(-20, ps4ApplyDeadzone(-20));
}

void test_esp32_center_packet_is_stop() {
	int lx = 0;
	int ly = 0;
	TEST_ASSERT_TRUE(ps4UpdateLeftStick(6127, 7127, lx, ly));
	const Ps4TankOutput out = ps4TankFromStick(lx, ly);
	TEST_ASSERT_TRUE(out.stop);
	TEST_ASSERT_EQUAL_INT(0, ps4ApplyDeadzone(lx));
	TEST_ASSERT_EQUAL_INT(0, ps4ApplyDeadzone(ly));
}

void test_forward_stick_drives_both_wheels() {
	int lx = 0;
	int ly = 0;
	TEST_ASSERT_TRUE(ps4UpdateLeftStick(6128, 7128 + 50, lx, ly));
	const Ps4TankOutput out = ps4TankFromStick(lx, ly);
	TEST_ASSERT_FALSE(out.stop);
	TEST_ASSERT_EQUAL_INT(50, out.leftVel);
	TEST_ASSERT_EQUAL_INT(50, out.rightVel);
}

void test_release_to_center_stops() {
	int lx = 80;
	int ly = 80;
	TEST_ASSERT_TRUE(ps4UpdateLeftStick(6128, 7128, lx, ly));
	const Ps4TankOutput out = ps4TankFromStick(lx, ly);
	TEST_ASSERT_TRUE(out.stop);
	TEST_ASSERT_EQUAL_INT(0, out.leftVel);
	TEST_ASSERT_EQUAL_INT(0, out.rightVel);
}

void test_single_axis_keeps_other() {
	int lx = 40;
	int ly = 60;
	TEST_ASSERT_TRUE(ps4UpdateLeftStick(6128, 6128, lx, ly));
	TEST_ASSERT_EQUAL_INT(0, lx);
	TEST_ASSERT_EQUAL_INT(60, ly);
}

void test_pwm_scale_and_clamp() {
	TEST_ASSERT_EQUAL_INT(0, ps4PwmFromVel(0));
	TEST_ASSERT_EQUAL_INT(100, ps4PwmFromVel(50));
	TEST_ASSERT_EQUAL_INT(100, ps4PwmFromVel(-50));
	TEST_ASSERT_EQUAL_INT(255, ps4PwmFromVel(200));
}

void test_drive_timeout() {
	TEST_ASSERT_FALSE(ps4DriveTimedOut(100, 0, 150));
	TEST_ASSERT_TRUE(ps4DriveTimedOut(150, 0, 150));
	TEST_ASSERT_TRUE(ps4DriveTimedOut(200, 0, 150));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_deadzone_zeros_center_noise);
	RUN_TEST(test_esp32_center_packet_is_stop);
	RUN_TEST(test_forward_stick_drives_both_wheels);
	RUN_TEST(test_release_to_center_stops);
	RUN_TEST(test_single_axis_keeps_other);
	RUN_TEST(test_pwm_scale_and_clamp);
	RUN_TEST(test_drive_timeout);
	return UNITY_END();
}

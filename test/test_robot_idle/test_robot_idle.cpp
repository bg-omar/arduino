#include <unity.h>
#include "robot_idle.h"

void test_robot_idle_head_sway_range() {
	RobotIdleState st{};
	const RobotIdleOutput a = robotIdleStep(st, 0, RobotIdleConfig{}, false);
	const RobotIdleOutput b = robotIdleStep(st, 1000, RobotIdleConfig{}, false);
	TEST_ASSERT_INT_WITHIN(5, ROBOT_IDLE_CENTER_XY, a.headXY);
	TEST_ASSERT_TRUE(b.headXY >= ROBOT_IDLE_CENTER_XY - ROBOT_IDLE_AMP_XY - 1);
	TEST_ASSERT_TRUE(b.headXY <= ROBOT_IDLE_CENTER_XY + ROBOT_IDLE_AMP_XY + 1);
}

void test_robot_idle_track_pulse_timing() {
	RobotIdleState st{};
	RobotIdleConfig cfg{};
	cfg.trackIntervalMs = 100;
	cfg.trackPulseMs = 20;
	bool sawPulse = false;
	for (uint32_t t = 0; t <= 120; t += 10) {
		const RobotIdleOutput out = robotIdleStep(st, t, cfg, true);
		if (out.trackPulse != RobotTrackPulse::None) {
			sawPulse = true;
			TEST_ASSERT_TRUE(out.trackPwm > 0);
		}
	}
	TEST_ASSERT_TRUE(sawPulse);
}

void test_robot_idle_no_track_when_disabled() {
	RobotIdleState st{};
	const RobotIdleOutput out = robotIdleStep(st, 5000, RobotIdleConfig{}, false);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(RobotTrackPulse::None), static_cast<int>(out.trackPulse));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_robot_idle_head_sway_range);
	RUN_TEST(test_robot_idle_track_pulse_timing);
	RUN_TEST(test_robot_idle_no_track_when_disabled);
	return UNITY_END();
}

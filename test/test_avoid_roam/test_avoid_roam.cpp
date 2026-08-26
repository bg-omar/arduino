#include <unity.h>
#include "avoid_roam.h"

void test_avoid_roam_motor_from_light() {
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::Forward),
		static_cast<int>(avoidRoamMotorFromLight(LightDriveIntent::CreepForward)));
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::TurnLeft),
		static_cast<int>(avoidRoamMotorFromLight(LightDriveIntent::CreepLeft)));
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::TurnRight),
		static_cast<int>(avoidRoamMotorFromLight(LightDriveIntent::CreepRight)));
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::Stop),
		static_cast<int>(avoidRoamMotorFromLight(LightDriveIntent::None)));
}

void test_avoid_roam_light_priority_over_wander() {
	AvoidRoamState state{};
	state.wandering = true;
	state.wanderLeft = true;
	state.wanderUntilMs = 5000;
	const AvoidRoamMotor motor = avoidRoamHostedMotor(
		1000, state, 100, 700, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::TurnRight), static_cast<int>(motor));
	TEST_ASSERT_FALSE(state.wandering);
}

void test_avoid_roam_dance_stops_motor() {
	AvoidRoamState state{};
	state.danceUntilMs = 3000;
	const AvoidRoamMotor motor = avoidRoamHostedMotor(
		1000, state, 0, 0, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::Stop), static_cast<int>(motor));
}

void test_avoid_roam_dance_head_axis_steps() {
	AvoidRoamState state{};
	state.danceUntilMs = 5000;
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamHeadAxis::XY),
		static_cast<int>(avoidRoamDanceHeadAxis(1000, state)));
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamHeadAxis::None),
		static_cast<int>(avoidRoamDanceHeadAxis(1200, state)));
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamHeadAxis::Z),
		static_cast<int>(avoidRoamDanceHeadAxis(1500, state)));
}

void test_avoid_roam_starts_dance_burst() {
	AvoidRoamState state{};
	state.danceNextMs = 1000;
	const AvoidRoamMotor motor = avoidRoamHostedMotor(
		1000, state, 0, 0, 10, 99, 0, 0, 0);
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::Stop), static_cast<int>(motor));
	TEST_ASSERT_TRUE(state.danceUntilMs > 1000);
}

void test_avoid_roam_forward_default() {
	AvoidRoamState state{};
	state.wanderNextMs = 0;
	state.danceNextMs = 5000;
	const AvoidRoamMotor motor = avoidRoamHostedMotor(
		1000, state, 0, 0, 99, 99, 0, 0, 0);
	TEST_ASSERT_EQUAL(static_cast<int>(AvoidRoamMotor::Forward), static_cast<int>(motor));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_avoid_roam_motor_from_light);
	RUN_TEST(test_avoid_roam_light_priority_over_wander);
	RUN_TEST(test_avoid_roam_dance_stops_motor);
	RUN_TEST(test_avoid_roam_dance_head_axis_steps);
	RUN_TEST(test_avoid_roam_starts_dance_burst);
	RUN_TEST(test_avoid_roam_forward_default);
	return UNITY_END();
}

#include <unity.h>
#include "light_drive.h"

void test_light_drive_forward_both_bright() {
	const SenseThresholds t = senseDefaultThresholds();
	TEST_ASSERT_EQUAL_INT(static_cast<int>(LightDriveIntent::CreepForward),
		static_cast<int>(lightDriveIntent(700, 720, t)));
}

void test_light_drive_right() {
	const SenseThresholds t = senseDefaultThresholds();
	TEST_ASSERT_EQUAL_INT(static_cast<int>(LightDriveIntent::CreepRight),
		static_cast<int>(lightDriveIntent(100, 700, t)));
}

void test_light_drive_none() {
	const SenseThresholds t = senseDefaultThresholds();
	TEST_ASSERT_EQUAL_INT(static_cast<int>(LightDriveIntent::None),
		static_cast<int>(lightDriveIntent(100, 130, t)));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_light_drive_forward_both_bright);
	RUN_TEST(test_light_drive_right);
	RUN_TEST(test_light_drive_none);
	return UNITY_END();
}

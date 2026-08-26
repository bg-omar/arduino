#include <unity.h>
#include "oled_mode.h"

void test_oled_mode_allows_draw() {
	TEST_ASSERT_TRUE(oledModeAllowsDraw(OledMode::Log, OledMode::Log));
	TEST_ASSERT_FALSE(oledModeAllowsDraw(OledMode::Menu, OledMode::Log));
	TEST_ASSERT_FALSE(oledModeAllowsDraw(OledMode::Pet, OledMode::Menu));
	TEST_ASSERT_TRUE(oledModeAllowsDraw(OledMode::Sensors, OledMode::Sensors));
	TEST_ASSERT_FALSE(oledModeAllowsDraw(OledMode::Log, OledMode::Sensors));
}

void test_log_only_when_enabled_and_log_mode() {
	TEST_ASSERT_TRUE(oledMayDrawLog(OledMode::Log, true));
	TEST_ASSERT_FALSE(oledMayDrawLog(OledMode::Log, false));
	TEST_ASSERT_FALSE(oledMayDrawLog(OledMode::Menu, true));
	TEST_ASSERT_FALSE(oledMayDrawLog(OledMode::Pet, true));
	TEST_ASSERT_FALSE(oledMayDrawLog(OledMode::Sensors, true));
}

void test_menu_only_when_dirty() {
	TEST_ASSERT_TRUE(oledMayDrawMenu(OledMode::Menu, true));
	TEST_ASSERT_FALSE(oledMayDrawMenu(OledMode::Menu, false));
	TEST_ASSERT_FALSE(oledMayDrawMenu(OledMode::Log, true));
}

void test_pet_only_in_pet_mode() {
	TEST_ASSERT_TRUE(oledMayDrawPet(OledMode::Pet));
	TEST_ASSERT_FALSE(oledMayDrawPet(OledMode::Log));
	TEST_ASSERT_FALSE(oledMayDrawPet(OledMode::None));
}

void test_sensors_only_when_enabled() {
	TEST_ASSERT_TRUE(oledMayDrawSensors(OledMode::Sensors, true));
	TEST_ASSERT_FALSE(oledMayDrawSensors(OledMode::Sensors, false));
	TEST_ASSERT_FALSE(oledMayDrawSensors(OledMode::Log, true));
	TEST_ASSERT_FALSE(oledMayDrawSensors(OledMode::Menu, true));
}

void test_sense_react_and_avoid_draw_gates() {
	TEST_ASSERT_TRUE(oledMayDrawSenseReact(OledMode::SenseReact));
	TEST_ASSERT_FALSE(oledMayDrawSenseReact(OledMode::Log));
	TEST_ASSERT_TRUE(oledMayDrawAvoid(OledMode::Avoid));
	TEST_ASSERT_FALSE(oledMayDrawAvoid(OledMode::SenseReact));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_oled_mode_allows_draw);
	RUN_TEST(test_log_only_when_enabled_and_log_mode);
	RUN_TEST(test_menu_only_when_dirty);
	RUN_TEST(test_pet_only_in_pet_mode);
	RUN_TEST(test_sensors_only_when_enabled);
	RUN_TEST(test_sense_react_and_avoid_draw_gates);
	return UNITY_END();
}

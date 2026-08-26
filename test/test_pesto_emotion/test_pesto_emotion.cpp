#include <unity.h>
#include "pesto_emotion.h"

void test_drive_forward_happy() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Happy),
						  static_cast<int>(pestoEmotionFromDrive(0, 50)));
}

void test_drive_backward_sad() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Sad),
						  static_cast<int>(pestoEmotionFromDrive(0, -50)));
}

void test_drive_left_look_left() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookLeft),
						  static_cast<int>(pestoEmotionFromDrive(-60, 10)));
}

void test_drive_right_look_right() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookRight),
						  static_cast<int>(pestoEmotionFromDrive(60, 10)));
}

void test_drive_deadzone_idle() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Idle),
						  static_cast<int>(pestoEmotionFromDrive(10, -5)));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Idle),
						  static_cast<int>(pestoEmotionFromDrive(0, 0)));
}

void test_head_look_up() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookUp),
						  static_cast<int>(pestoEmotionFromHead(0, -50)));
}

void test_head_look_down() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookDown),
						  static_cast<int>(pestoEmotionFromHead(0, 50)));
}

void test_button_triangle_heart() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Heart),
						  static_cast<int>(pestoEmotionFromButton(3400)));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Heart),
						  static_cast<int>(pestoEmotionFromButton(3410)));
}

void test_button_l3_alert() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Alert),
						  static_cast<int>(pestoEmotionFromButton(2300)));
}

void test_button_circle_pet_status() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Happy),
						  static_cast<int>(pestoEmotionFromButton(3300, true)));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Sad),
						  static_cast<int>(pestoEmotionFromButton(3300, false)));
}

void test_button_unknown_idle() {
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Idle),
						  static_cast<int>(pestoEmotionFromButton(2500)));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Idle),
						  static_cast<int>(pestoEmotionFromButton(9999)));
}

void test_pick_button_beats_drive() {
	const PestoEmotion e = pestoPickEmotion(3400, 0, 80, 0, 0);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Heart), static_cast<int>(e));
}

void test_pick_drive_beats_head() {
	const PestoEmotion e = pestoPickEmotion(0, 0, 80, -90, 0);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Happy), static_cast<int>(e));
}

void test_pick_head_when_idle_drive() {
	const PestoEmotion e = pestoPickEmotion(0, 0, 0, 0, -50);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookUp), static_cast<int>(e));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_drive_forward_happy);
	RUN_TEST(test_drive_backward_sad);
	RUN_TEST(test_drive_left_look_left);
	RUN_TEST(test_drive_right_look_right);
	RUN_TEST(test_drive_deadzone_idle);
	RUN_TEST(test_head_look_up);
	RUN_TEST(test_head_look_down);
	RUN_TEST(test_button_triangle_heart);
	RUN_TEST(test_button_l3_alert);
	RUN_TEST(test_button_circle_pet_status);
	RUN_TEST(test_button_unknown_idle);
	RUN_TEST(test_pick_button_beats_drive);
	RUN_TEST(test_pick_drive_beats_head);
	RUN_TEST(test_pick_head_when_idle_drive);
	return UNITY_END();
}

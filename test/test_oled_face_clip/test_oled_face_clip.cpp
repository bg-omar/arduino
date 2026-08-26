#include <unity.h>
#include "oled_face_clip.h"

void test_oled_face_clip_meta() {
	const OledFaceClipMeta idle = oledFaceClipMeta(OledFaceClipId::IdleBlink);
	TEST_ASSERT_EQUAL_UINT8(12, idle.frameCount);
	TEST_ASSERT_EQUAL_UINT16(125, idle.frameIntervalMs);
}

void test_oled_face_advance_wrap() {
	const uint8_t next = oledFaceAdvanceFrame(OledFaceClipId::LookLR, 7, 1000, 0);
	TEST_ASSERT_EQUAL_UINT8(0, next);
}

void test_oled_face_for_sense_seek() {
	const OledFaceClipId clip = oledFaceForSense(PestoEmotion::Idle, SenseTrigger::None, true);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(OledFaceClipId::Happy), static_cast<int>(clip));
}

void test_oled_face_look_lr_offset() {
	TEST_ASSERT_EQUAL_UINT8(0, oledFaceDisplayFrame(OledFaceClipId::LookLR, 4, PestoEmotion::LookLeft));
	TEST_ASSERT_EQUAL_UINT8(4, oledFaceDisplayFrame(OledFaceClipId::LookLR, 0, PestoEmotion::LookRight));
	TEST_ASSERT_EQUAL_UINT8(3, oledFaceDisplayFrame(OledFaceClipId::IdleBlink, 3, PestoEmotion::LookLeft));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_oled_face_clip_meta);
	RUN_TEST(test_oled_face_advance_wrap);
	RUN_TEST(test_oled_face_for_sense_seek);
	RUN_TEST(test_oled_face_look_lr_offset);
	return UNITY_END();
}

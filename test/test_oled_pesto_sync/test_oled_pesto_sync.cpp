#include <unity.h>
#include <cstring>
#include "oled_pesto_sync.h"

static void fillOledRegion(uint8_t* oled1024, int x0, int y0, int w, int h) {
	std::memset(oled1024, 0, 1024);
	for (int y = y0; y < y0 + h; ++y) {
		for (int x = x0; x < x0 + w; ++x) {
			const int byteIdx = y * 16 + x / 8;
			const int bit = 7 - (x % 8);
			oled1024[byteIdx] |= static_cast<uint8_t>(1u << bit);
		}
	}
}

void test_oled_downsample_all_black() {
	uint8_t oled[1024];
	uint8_t out[16];
	std::memset(oled, 0, sizeof(oled));
	oledBitmapDownsample16x8(oled, out);
	for (int i = 0; i < 16; ++i) {
		TEST_ASSERT_EQUAL_UINT8(0, out[i]);
	}
}

void test_oled_downsample_all_white() {
	uint8_t oled[1024];
	uint8_t out[16];
	std::memset(oled, 0xFF, sizeof(oled));
	oledBitmapDownsample16x8(oled, out);
	for (int i = 0; i < 16; ++i) {
		TEST_ASSERT_EQUAL_UINT8(0xFF, out[i]);
	}
}

void test_oled_downsample_top_left_block() {
	uint8_t oled[1024];
	uint8_t out[16];
	fillOledRegion(oled, 0, 0, 16, 16);
	oledBitmapDownsample16x8(oled, out);
	TEST_ASSERT_EQUAL_UINT8(0x03, out[0]);
	TEST_ASSERT_EQUAL_UINT8(0x03, out[1]);
	TEST_ASSERT_EQUAL_UINT8(0, out[2]);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_oled_downsample_all_black);
	RUN_TEST(test_oled_downsample_all_white);
	RUN_TEST(test_oled_downsample_top_left_block);
	return UNITY_END();
}

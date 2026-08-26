#include <unity.h>
#include "mic_peak.h"

void test_mic_peak_reset() {
	MicPeakState s;
	micPeakSample(s, 500, 600);
	micPeakReset(s);
	TEST_ASSERT_EQUAL_UINT16(0, s.maxL);
	TEST_ASSERT_EQUAL_UINT16(1024, s.minL);
	TEST_ASSERT_EQUAL_UINT16(0, s.maxR);
	TEST_ASSERT_EQUAL_UINT16(1024, s.minR);
}

void test_mic_peak_tracks_min_max() {
	MicPeakState s;
	micPeakSample(s, 100, 200);
	micPeakSample(s, 900, 50);
	micPeakSample(s, 300, 800);
	TEST_ASSERT_EQUAL_UINT16(900, s.maxL);
	TEST_ASSERT_EQUAL_UINT16(100, s.minL);
	TEST_ASSERT_EQUAL_UINT16(800, s.maxR);
	TEST_ASSERT_EQUAL_UINT16(50, s.minR);
	TEST_ASSERT_EQUAL_UINT16(800, micPeakToPeak(s.maxL, s.minL));
	TEST_ASSERT_EQUAL_UINT16(750, micPeakToPeak(s.maxR, s.minR));
}

void test_mic_peak_ignores_invalid_samples() {
	MicPeakState s;
	micPeakSample(s, 2000, 3000);
	TEST_ASSERT_EQUAL_UINT16(0, s.maxL);
	TEST_ASSERT_EQUAL_UINT16(1024, s.minL);
	TEST_ASSERT_EQUAL_UINT16(0, s.maxR);
	TEST_ASSERT_EQUAL_UINT16(1024, s.minR);
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
	UNITY_BEGIN();
	RUN_TEST(test_mic_peak_reset);
	RUN_TEST(test_mic_peak_tracks_min_max);
	RUN_TEST(test_mic_peak_ignores_invalid_samples);
	return UNITY_END();
}

#include <unity.h>
#include "compass_heading.h"
#include <cmath>

void test_normalize_wraps() {
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, normalizeDegrees360(370.0f));
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 350.0f, normalizeDegrees360(-10.0f));
	TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, normalizeDegrees360(720.0f));
}

void test_heading_xz_axes() {
	// +X → 0°
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, compassHeadingDegrees(1.0f, 0.0f));
	// +Z → 90°
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, compassHeadingDegrees(0.0f, 1.0f));
	// -X → 180°
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 180.0f, compassHeadingDegrees(-1.0f, 0.0f));
	// -Z → 270°
	TEST_ASSERT_FLOAT_WITHIN(0.5f, 270.0f, compassHeadingDegrees(0.0f, -1.0f));
}

void test_cardinal_bands() {
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::N), static_cast<int>(cardinalFromDegrees(0.0f)));
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::N), static_cast<int>(cardinalFromDegrees(44.0f)));
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::E), static_cast<int>(cardinalFromDegrees(45.0f)));
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::E), static_cast<int>(cardinalFromDegrees(90.0f)));
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::S), static_cast<int>(cardinalFromDegrees(180.0f)));
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::W), static_cast<int>(cardinalFromDegrees(270.0f)));
	TEST_ASSERT_EQUAL(static_cast<int>(CompassCardinal::N), static_cast<int>(cardinalFromDegrees(315.0f)));
}

void test_cardinal_string() {
	TEST_ASSERT_EQUAL_STRING("N", cardinalToString(CompassCardinal::N));
	TEST_ASSERT_EQUAL_STRING("E", cardinalToString(CompassCardinal::E));
	TEST_ASSERT_EQUAL_STRING("S", cardinalToString(CompassCardinal::S));
	TEST_ASSERT_EQUAL_STRING("W", cardinalToString(CompassCardinal::W));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_normalize_wraps);
	RUN_TEST(test_heading_xz_axes);
	RUN_TEST(test_cardinal_bands);
	RUN_TEST(test_cardinal_string);
	return UNITY_END();
}

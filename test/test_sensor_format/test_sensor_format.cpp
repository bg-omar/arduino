#include <unity.h>
#include "sensor_format.h"
#include <cstring>

void test_heading_line() {
	char line[18];
	formatHeadingLine(line, sizeof(line), 180.4, "S");
	TEST_ASSERT_EQUAL_STRING("180 S", line);
}

void test_accel_gyro_lines() {
	char line[24];
	formatAccelLine(line, sizeof(line), 1.0f, -2.5f, 9.8f);
	TEST_ASSERT_EQUAL_STRING("A1.0 -2.5 9.8", line);
	formatGyroLine(line, sizeof(line), 0.1f, 0.2f, -0.3f);
	TEST_ASSERT_EQUAL_STRING("G0.1 0.2 -0.3", line);
}

void test_accel_gyro_compact() {
	char line[40];
	formatAccelGyroCompact(line, sizeof(line), 1.0f, 2.0f, 3.0f, 0.1f, 0.2f, 0.3f);
	TEST_ASSERT_EQUAL_STRING("A1.0 2.0 3.0 G0.1 0.2 0.3", line);
}

void test_baro_mic_light_dist() {
	char line[24];
	formatBaroLine(line, sizeof(line), 21.5f, 1013.2f);
	TEST_ASSERT_EQUAL_STRING("21.5C 1013hPa", line);
	formatMicLine(line, sizeof(line), 12, 34);
	TEST_ASSERT_EQUAL_STRING("M12/34", line);
	formatLightLine(line, sizeof(line), 100, 200);
	TEST_ASSERT_EQUAL_STRING("Lt100/200", line);
	formatDistLine(line, sizeof(line), 42.6);
	TEST_ASSERT_EQUAL_STRING("43 cm", line);
}

void test_mic_light_and_baro_dist_compact() {
	char line[32];
	formatMicLightCompact(line, sizeof(line), 1, 2, 3, 4);
	TEST_ASSERT_EQUAL_STRING("M1/2 Lt3/4", line);
	formatBaroDistCompact(line, sizeof(line), 20.0f, 1000.0f, 15.0);
	TEST_ASSERT_EQUAL_STRING("20.0C 1000h 15cm", line);
}

void test_impact_lines() {
	char line[48];
	formatImpactAccelGyro(line, sizeof(line), 1.0f, 2.0f, 3.0f, 0.1f, 0.2f, 0.3f);
	TEST_ASSERT_EQUAL_STRING("IMP A1.0,2.0,3.0 G0.1,0.2,0.3", line);
	formatImpactBaro(line, sizeof(line), 22.0f, 1012.0f);
	TEST_ASSERT_EQUAL_STRING(" B T22.0 P1012", line);
	formatImpactMicLight(line, sizeof(line), 10, 20, 30, 40);
	TEST_ASSERT_EQUAL_STRING(" M10/20 L30/40", line);
	formatImpactDist(line, sizeof(line), 55.0);
	TEST_ASSERT_EQUAL_STRING(" D55", line);
}

void test_dist_meters_and_bars() {
	char line[16];
	formatDistMeters(line, sizeof(line), -1.0);
	TEST_ASSERT_EQUAL_STRING("-- m", line);
	formatDistMeters(line, sizeof(line), 240.0);
	TEST_ASSERT_EQUAL_STRING("2.4 m", line);
	formatDistMeters(line, sizeof(line), 600.0);
	TEST_ASSERT_EQUAL_STRING("5.0 m", line);
	TEST_ASSERT_EQUAL(20, sensorBarPx(1024, 2048, 40));
	TEST_ASSERT_EQUAL(50, distDisplayBarPx(500.0));
	TEST_ASSERT_EQUAL(0, distDisplayBarPx(-1.0));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_heading_line);
	RUN_TEST(test_accel_gyro_lines);
	RUN_TEST(test_accel_gyro_compact);
	RUN_TEST(test_baro_mic_light_dist);
	RUN_TEST(test_mic_light_and_baro_dist_compact);
	RUN_TEST(test_impact_lines);
	RUN_TEST(test_dist_meters_and_bars);
	return UNITY_END();
}

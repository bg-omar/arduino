#include <unity.h>
#include <cmath>
#include "radar_scan.h"

void test_radar_pan_tilt_from_servo() {
	TEST_ASSERT_FLOAT_WITHIN(0.1f, 90.0f, radarPanDegFromServo(0));
	TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, radarPanDegFromServo(90));
	TEST_ASSERT_FLOAT_WITHIN(0.1f, -90.0f, radarPanDegFromServo(180));
	TEST_ASSERT_FLOAT_WITHIN(0.1f, -45.0f, radarTiltDegFromServo(135));
	TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, radarTiltDegFromServo(90));
	TEST_ASSERT_FLOAT_WITHIN(0.1f, 90.0f, radarTiltDegFromServo(0));
}

void test_radar_xyz_forward_right_up() {
	const RadarXYZ fwd = radarDistToXYZ(100.0, 0.0f, 0.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 0.0, fwd.x);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 100.0, fwd.y);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 0.0, fwd.z);

	const RadarXYZ right = radarDistToXYZ(100.0, 90.0f, 0.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 100.0, right.x);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 0.0, right.y);

	const RadarXYZ up = radarDistToXYZ(100.0, 0.0f, 90.0f);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 100.0, up.z);
	TEST_ASSERT_FLOAT_WITHIN(1.0, 0.0, up.y);
}

void test_radar_grid_dimensions() {
	TEST_ASSERT_EQUAL(19, RADAR_PAN_COLS);
	TEST_ASSERT_EQUAL(10, RADAR_TILT_ROWS);
	TEST_ASSERT_EQUAL(190, RADAR_CELL_COUNT);
}

void test_radar_zigzag_cols() {
	TEST_ASSERT_EQUAL(0, radarColForCell(0, 0));
	TEST_ASSERT_EQUAL(18, radarColForCell(18, 0));
	TEST_ASSERT_EQUAL(18, radarColForCell(19, 1));
	TEST_ASSERT_EQUAL(0, radarColForCell(37, 1));
}

void test_radar_cell_servo_positions() {
	int xy = 0;
	int z = 0;
	radarCellServo(0, xy, z);
	TEST_ASSERT_EQUAL(0, xy);
	TEST_ASSERT_EQUAL(135, z);
	radarCellServo(18, xy, z);
	TEST_ASSERT_EQUAL(180, xy);
	TEST_ASSERT_EQUAL(135, z);
	radarCellServo(19, xy, z);
	TEST_ASSERT_EQUAL(180, xy);
	TEST_ASSERT_EQUAL(120, z);
}

void test_radar_distance_valid() {
	TEST_ASSERT_FALSE(radarDistanceValid(-1.0));
	TEST_ASSERT_TRUE(radarDistanceValid(0.0));
	TEST_ASSERT_TRUE(radarDistanceValid(42.5));
	TEST_ASSERT_TRUE(radarDistanceValid(500.0));
	TEST_ASSERT_FALSE(radarDistanceValid(500.1));
}

void test_radar_ping_timeout_is_500cm() {
	TEST_ASSERT_EQUAL(500, RADAR_MAX_CM);
	TEST_ASSERT_EQUAL_UINT32(29000u, RADAR_PING_TIMEOUT_US);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_radar_pan_tilt_from_servo);
	RUN_TEST(test_radar_xyz_forward_right_up);
	RUN_TEST(test_radar_grid_dimensions);
	RUN_TEST(test_radar_zigzag_cols);
	RUN_TEST(test_radar_cell_servo_positions);
	RUN_TEST(test_radar_distance_valid);
	RUN_TEST(test_radar_ping_timeout_is_500cm);
	return UNITY_END();
}

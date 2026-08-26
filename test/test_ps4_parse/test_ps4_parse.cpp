#include <unity.h>
#include "ps4_parse.h"

void test_single_button() {
	int values[2] = {0, 0};
	uint8_t count = 0;
	TEST_ASSERT_TRUE(parsePs4Message("1100", values, &count));
	TEST_ASSERT_EQUAL_UINT8(1, count);
	TEST_ASSERT_EQUAL_INT(1100, values[0]);
}

void test_dual_stick() {
	int values[2] = {0, 0};
	uint8_t count = 0;
	TEST_ASSERT_TRUE(parsePs4Message("6127+7127", values, &count));
	TEST_ASSERT_EQUAL_UINT8(2, count);
	TEST_ASSERT_EQUAL_INT(6127, values[0]);
	TEST_ASSERT_EQUAL_INT(7127, values[1]);
}

void test_l2_r2() {
	int values[2] = {0, 0};
	uint8_t count = 0;
	TEST_ASSERT_TRUE(parsePs4Message("4000+5000", values, &count));
	TEST_ASSERT_EQUAL_UINT8(2, count);
	TEST_ASSERT_EQUAL_INT(4000, values[0]);
	TEST_ASSERT_EQUAL_INT(5000, values[1]);
}

void test_letters_returns_zero_pair() {
	int values[2] = {-1, -1};
	uint8_t count = 0;
	TEST_ASSERT_TRUE(parsePs4Message("abc", values, &count));
	TEST_ASSERT_EQUAL_UINT8(2, count);
	TEST_ASSERT_EQUAL_INT(0, values[0]);
	TEST_ASSERT_EQUAL_INT(0, values[1]);
}

void test_trailing_cr() {
	int values[2] = {0, 0};
	uint8_t count = 0;
	TEST_ASSERT_TRUE(parsePs4Message("6127+7127 \r", values, &count));
	TEST_ASSERT_EQUAL_UINT8(2, count);
	TEST_ASSERT_EQUAL_INT(6127, values[0]);
	TEST_ASSERT_EQUAL_INT(7127, values[1]);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_single_button);
	RUN_TEST(test_dual_stick);
	RUN_TEST(test_l2_r2);
	RUN_TEST(test_letters_returns_zero_pair);
	RUN_TEST(test_trailing_cr);
	return UNITY_END();
}

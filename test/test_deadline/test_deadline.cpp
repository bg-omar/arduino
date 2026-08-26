#include <unity.h>
#include "deadline.h"

void test_elapsed_not_yet() {
	uint32_t last = 100;
	TEST_ASSERT_FALSE(elapsed(149, last, 50));
	TEST_ASSERT_EQUAL_UINT32(100, last);
}

void test_elapsed_at_interval() {
	uint32_t last = 100;
	TEST_ASSERT_TRUE(elapsed(150, last, 50));
	TEST_ASSERT_EQUAL_UINT32(150, last);
}

void test_elapsed_after_interval() {
	uint32_t last = 100;
	TEST_ASSERT_TRUE(elapsed(200, last, 50));
	TEST_ASSERT_EQUAL_UINT32(200, last);
}

void test_elapsed_wrap_around() {
	uint32_t last = 0xFFFFFFE0u;
	TEST_ASSERT_FALSE(elapsed(0x10u, last, 50));
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFE0u, last);
	TEST_ASSERT_TRUE(elapsed(0x20u, last, 50));
	TEST_ASSERT_EQUAL_UINT32(0x20u, last);
}

void test_deadline_not_due() {
	Deadline d;
	d.after(1000, 500);
	TEST_ASSERT_FALSE(d.isDue(1499));
}

void test_deadline_is_due() {
	Deadline d;
	d.after(1000, 500);
	TEST_ASSERT_TRUE(d.isDue(1500));
	TEST_ASSERT_TRUE(d.isDue(1600));
}

void test_deadline_immediate() {
	Deadline d;
	d.after(42, 0);
	TEST_ASSERT_TRUE(d.isDue(42));
}

void test_deadline_wrap_around() {
	Deadline d;
	d.after(0xFFFFFFFAu, 10);
	TEST_ASSERT_FALSE(d.isDue(0xFFFFFFFEu));
	TEST_ASSERT_TRUE(d.isDue(5));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_elapsed_not_yet);
	RUN_TEST(test_elapsed_at_interval);
	RUN_TEST(test_elapsed_after_interval);
	RUN_TEST(test_elapsed_wrap_around);
	RUN_TEST(test_deadline_not_due);
	RUN_TEST(test_deadline_is_due);
	RUN_TEST(test_deadline_immediate);
	RUN_TEST(test_deadline_wrap_around);
	return UNITY_END();
}

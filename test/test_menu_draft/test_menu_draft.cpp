#include <unity.h>
#include "menu_draft.h"

void test_snapshot_restore() {
	bool a = true;
	bool b = false;
	bool c = true;
	bool* flags[3] = {&a, &b, &c};
	bool backup[3] = {};

	menuDraftSnapshot(backup, flags, 3);
	TEST_ASSERT_TRUE(backup[0]);
	TEST_ASSERT_FALSE(backup[1]);
	TEST_ASSERT_TRUE(backup[2]);

	a = false;
	b = true;
	c = false;
	TEST_ASSERT_FALSE(menuDraftEquals(flags, backup, 3));

	menuDraftRestore(flags, backup, 3);
	TEST_ASSERT_TRUE(a);
	TEST_ASSERT_FALSE(b);
	TEST_ASSERT_TRUE(c);
	TEST_ASSERT_TRUE(menuDraftEquals(flags, backup, 3));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_snapshot_restore);
	return UNITY_END();
}

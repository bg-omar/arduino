#include <unity.h>
#include "shared_control.h"
#include "user_mode_protocol.h"

void setUp() {}
void tearDown() {}

void test_ps4_override_is_temporary() {
    TEST_ASSERT_EQUAL_INT((int)WallzControlOwner::Brain, (int)wallzControlOwner(true,false,false));
    TEST_ASSERT_EQUAL_INT((int)WallzControlOwner::Ps4Manual, (int)wallzControlOwner(true,true,false));
    TEST_ASSERT_EQUAL_INT((int)WallzControlOwner::Brain, (int)wallzControlOwner(true,false,false));
}

void test_robot_mode_excludes_brain() {
    TEST_ASSERT_EQUAL_INT((int)WallzControlOwner::RobotMode, (int)wallzControlOwner(true,false,true));
}

void test_user_mode_protocol() {
    TEST_ASSERT_EQUAL_INT((int)RaUserModeRequest::Brain, (int)parseRaUserModeRequest("U,MODE,BRAIN"));
    TEST_ASSERT_EQUAL_INT((int)RaUserModeRequest::Imitation, (int)parseRaUserModeRequest("U,MODE,IMITATION"));
    TEST_ASSERT_EQUAL_INT((int)RaUserModeRequest::SenseReact, (int)parseRaUserModeRequest("U,MODE,SENSE"));
    TEST_ASSERT_EQUAL_INT((int)RaUserModeRequest::FreeRoam, (int)parseRaUserModeRequest("U,MODE,FREE_ROAM"));
    TEST_ASSERT_EQUAL_INT((int)RaUserModeRequest::Stop, (int)parseRaUserModeRequest("U,MODE,STOP"));
    TEST_ASSERT_EQUAL_INT((int)RaUserModeRequest::None, (int)parseRaUserModeRequest("A,ARM,1"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ps4_override_is_temporary);
    RUN_TEST(test_robot_mode_excludes_brain);
    RUN_TEST(test_user_mode_protocol);
    return UNITY_END();
}

#include <unity.h>
#include "sense_mood.h"

static SenseSnapshot baseSnap() {
	SenseSnapshot s{};
	s.distanceCm = -1.0f;
	s.headingDeg = -1.0f;
	s.baroDeltaAbs = 0.0f;
	return s;
}

void test_distance_alert_beats_mic() {
	SenseSnapshot s = baseSnap();
	s.hasDistance = true;
	s.distanceCm = 20.0f;
	s.hasMic = true;
	s.micL = 200;
	s.micR = 10;
	const SenseMoodResult r = sensePickMood(s);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Alert), static_cast<int>(r.emotion));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseMotorIntent::Back), static_cast<int>(r.motor));
}

void test_gyro_impact_alert() {
	SenseSnapshot s = baseSnap();
	s.hasGyro = true;
	s.gyroImpact = true;
	s.hasMic = true;
	s.micL = 200;
	s.micR = 0;
	const SenseMoodResult r = sensePickMood(s);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Alert), static_cast<int>(r.emotion));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseMotorIntent::Stop), static_cast<int>(r.motor));
}

void test_mic_left_look_left() {
	SenseSnapshot s = baseSnap();
	s.hasMic = true;
	s.micL = 120;
	s.micR = 20;
	const SenseMoodResult r = sensePickMood(s);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookLeft), static_cast<int>(r.emotion));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseMotorIntent::Left), static_cast<int>(r.motor));
}

void test_light_right_look_creep() {
	SenseSnapshot s = baseSnap();
	s.hasLight = true;
	s.lightL = 100;
	s.lightR = 700;
	const SenseMoodResult r = sensePickMood(s);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::LookRight), static_cast<int>(r.emotion));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseMotorIntent::CreepRight), static_cast<int>(r.motor));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseTrigger::Light), static_cast<int>(r.trigger));
}

void test_idle_snapshot() {
	SenseSnapshot s = baseSnap();
	const SenseMoodResult r = sensePickMood(s);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Idle), static_cast<int>(r.emotion));
	TEST_ASSERT_EQUAL_INT(static_cast<int>(SenseMotorIntent::Stop), static_cast<int>(r.motor));
}

void test_compass_heading_does_not_steal_idle() {
	SenseSnapshot s = baseSnap();
	s.hasCompass = true;
	s.headingDeg = 90.0f;
	const SenseMoodResult r = sensePickMood(s);
	TEST_ASSERT_EQUAL_INT(static_cast<int>(PestoEmotion::Idle), static_cast<int>(r.emotion));
	TEST_ASSERT_TRUE(r.isIdle);
	TEST_ASSERT_FALSE(r.headOnly);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_distance_alert_beats_mic);
	RUN_TEST(test_gyro_impact_alert);
	RUN_TEST(test_mic_left_look_left);
	RUN_TEST(test_light_right_look_creep);
	RUN_TEST(test_idle_snapshot);
	RUN_TEST(test_compass_heading_does_not_steal_idle);
	return UNITY_END();
}

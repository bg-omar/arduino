#include "robot_modes.h"

#include "avoid_objects.h"
#include "dancing.h"
#include "follow_light.h"
#include "motor.h"
#include "radar_scan.h"
#include "sense_react.h"

namespace robot_modes {

bool anyActive() {
	return avoid_objects::isActive()
		|| Follow_light::isActive()
		|| dancing::isActive()
		|| sense_react::isActive()
		|| radar_scan::isActive();
}

void stopAll() {
	avoid_objects::stop();
	Follow_light::stop();
	dancing::stop();
	sense_react::stop();
	radar_scan::stop();
	Motor::Car_Stop();
}

} // namespace robot_modes

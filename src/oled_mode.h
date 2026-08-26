//

// Priority mood from sensor snapshot. Arduino-free for native tests.

//



#ifndef OLED_MODE_H

#define OLED_MODE_H



#include <cstdint>



enum class OledMode : uint8_t {

	Log = 0,

	Menu = 1,

	Pet = 2,

	Sensors = 3,

	None = 4,

	AutonomyMenu = 5,

	SenseReact = 6,

	Avoid = 7,

};



inline bool oledModeAllowsDraw(OledMode current, OledMode required) {

	return current == required;

}



inline bool oledMayDrawLog(OledMode mode, bool oledEnabled) {

	return oledEnabled && oledModeAllowsDraw(mode, OledMode::Log);

}



inline bool oledMayDrawMenu(OledMode mode, bool menuDirty) {

	return oledModeAllowsDraw(mode, OledMode::Menu) && menuDirty;

}



inline bool oledMayDrawPet(OledMode mode) {

	return oledModeAllowsDraw(mode, OledMode::Pet);

}



inline bool oledMayDrawSensors(OledMode mode, bool sensorsEnabled) {

	return sensorsEnabled && oledModeAllowsDraw(mode, OledMode::Sensors);

}


inline bool oledMayDrawSenseReact(OledMode mode) {

	return oledModeAllowsDraw(mode, OledMode::SenseReact);

}



inline bool oledMayDrawAvoid(OledMode mode) {

	return oledModeAllowsDraw(mode, OledMode::Avoid);

}



#endif // OLED_MODE_H

//
// NTP clock for the onboard ESP32-S3 gateway.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_REAL_TIME_H
#define ARDUINO_R4_UNO_WALL_Z_REAL_TIME_H

#include <cstddef>

class real_time {
public:
	static void begin();
	static void poll();
	static bool synced();
	static void format(char* dest, size_t cap);
};

#endif // ARDUINO_R4_UNO_WALL_Z_REAL_TIME_H

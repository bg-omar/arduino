//
// Created by mr on 7/8/2024.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_LOGGER_H
#define ARDUINO_R4_UNO_WALL_Z_LOGGER_H


class logger {
public:
	static void log(const char* text) ;
	static void logln(const char* text);

	static void logInt(int id);
	static void logIntln(int id);

	static void logFloat(float floaty) ;
	static void logFloatln(float id) ;

	static void logHex(unsigned char id, int i);
	static void logHexln(unsigned char id, int i);

	static void logDoubble(double floaty);
	static void logDoubbleln(double floaty);

	static void endBootLog();
	static bool isOledEnabled();
};


#endif //ARDUINO_R4_UNO_WALL_Z_LOGGER_H

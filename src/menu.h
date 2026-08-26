//
// Created by mr on 6/12/2024.
//

#ifndef ARDUINO_R4_UNO_WALL_Z_MENU_H
#define ARDUINO_R4_UNO_WALL_Z_MENU_H

class menu {

public:
	static void loopMenu();
	static void up();
	static void down();
	static void select();
	static void open();
	static void toggle();
	static void closeDiscard();
	static void undo();
	static void save();
	static bool isOpen();
	static void toggleSensorsPage();
};

#endif //ARDUINO_R4_UNO_WALL_Z_MENU_H

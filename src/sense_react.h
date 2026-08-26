//
// Sense-react autonomous mode.
//

#ifndef SENSE_REACT_H
#define SENSE_REACT_H

class sense_react {
public:
	static void start();
	static void stop();
	static void tick();
	static bool isActive();

private:
	static bool active;
};

#endif // SENSE_REACT_H

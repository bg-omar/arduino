//
// OLED / log labels for sense triggers — Arduino-free.
//
#ifndef SENSE_TRIGGER_H
#define SENSE_TRIGGER_H

#include "sense_mood.h"

inline const char* senseTriggerLabel(SenseTrigger trigger, PestoEmotion emotion) {
	switch (trigger) {
		case SenseTrigger::Mic:
			if (emotion == PestoEmotion::LookLeft) {
				return "Mic L";
			}
			if (emotion == PestoEmotion::LookRight) {
				return "Mic R";
			}
			return "Mic";
		case SenseTrigger::Light:
			if (emotion == PestoEmotion::LookLeft) {
				return "Light L";
			}
			if (emotion == PestoEmotion::LookRight) {
				return "Light R";
			}
			return "Light +";
		case SenseTrigger::Distance:
			return "Near";
		case SenseTrigger::Gyro:
			return "Shock";
		case SenseTrigger::Baro:
			return "Baro";
		case SenseTrigger::Compass:
			return "Compass";
		case SenseTrigger::Seek:
			return "Seek";
		case SenseTrigger::Avoid:
			return "Avoid";
		case SenseTrigger::None:
		default:
			return "Idle";
	}
}

#endif // SENSE_TRIGGER_H

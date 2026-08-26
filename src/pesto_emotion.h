//
// Map PS4 drive / head / buttons to Pesto matrix emotions. Arduino-free for native tests.
//

#ifndef PESTO_EMOTION_H
#define PESTO_EMOTION_H

#include <cstdint>

#include "ps4_drive.h"

enum class PestoEmotion : uint8_t {
	Idle = 0,
	Happy,
	Sad,
	LookLeft,
	LookRight,
	LookUp,
	LookDown,
	Heart,
	Alert,
	Curious,
	Wink
};

constexpr uint32_t PESTO_EMOTION_HOLD_MS = 1200;

inline int pestoAbs(int v) {
	return v < 0 ? -v : v;
}

// Left stick / L2R2 tank axes (already centered, before or after deadzone).
inline PestoEmotion pestoEmotionFromDrive(int lx, int ly, int deadzone = PS4_STICK_DEADZONE) {
	const int x = ps4ApplyDeadzone(lx, deadzone);
	const int y = ps4ApplyDeadzone(ly, deadzone);
	if (x == 0 && y == 0) {
		return PestoEmotion::Idle;
	}
	if (pestoAbs(y) >= pestoAbs(x)) {
		return y > 0 ? PestoEmotion::Happy : PestoEmotion::Sad;
	}
	return x < 0 ? PestoEmotion::LookLeft : PestoEmotion::LookRight;
}

// Right stick head aim (centered deltas).
inline PestoEmotion pestoEmotionFromHead(int rx, int ry, int deadzone = PS4_STICK_DEADZONE) {
	const int x = ps4ApplyDeadzone(rx, deadzone);
	const int y = ps4ApplyDeadzone(ry, deadzone);
	if (x == 0 && y == 0) {
		return PestoEmotion::Idle;
	}
	if (pestoAbs(y) >= pestoAbs(x)) {
		// Stick up is negative RY (matches ps4_head invert convention).
		return y < 0 ? PestoEmotion::LookUp : PestoEmotion::LookDown;
	}
	return x < 0 ? PestoEmotion::LookLeft : PestoEmotion::LookRight;
}

// Button bases (xxxx) and event xx10 / xx01 — normalize to base / 100 * 100.
inline int pestoButtonBase(int code) {
	if (code < 1000 || code >= 4000) {
		return 0;
	}
	return (code / 100) * 100;
}

// petHappy: true when OLED petStatus == 0 (happy).
inline PestoEmotion pestoEmotionFromButton(int code, bool petHappy = true) {
	const int base = pestoButtonBase(code);
	switch (base) {
		case 3400: // Triangle
			return PestoEmotion::Heart;
		case 3300: // Circle
			return petHappy ? PestoEmotion::Happy : PestoEmotion::Sad;
		case 3200: // Cross
			return PestoEmotion::Happy;
		case 3100: // Square
			return PestoEmotion::Curious;
		case 1100: // D-pad Up
			return PestoEmotion::LookUp;
		case 1200: // D-pad Right
			return PestoEmotion::LookRight;
		case 1300: // D-pad Down
			return PestoEmotion::LookDown;
		case 1400: // D-pad Left
			return PestoEmotion::LookLeft;
		case 2300: // L3
			return PestoEmotion::Alert;
		case 2400: // R3
			return PestoEmotion::Curious;
		case 2100: // L1
		case 2200: // R1
			return PestoEmotion::Wink;
		default:
			return PestoEmotion::Idle;
	}
}

// Priority: button > drive > head > idle.
inline PestoEmotion pestoPickEmotion(int buttonCode, int lx, int ly, int rx, int ry,
									bool petHappy = true, int deadzone = PS4_STICK_DEADZONE) {
	const PestoEmotion fromButton = pestoEmotionFromButton(buttonCode, petHappy);
	if (fromButton != PestoEmotion::Idle) {
		return fromButton;
	}
	const PestoEmotion fromDrive = pestoEmotionFromDrive(lx, ly, deadzone);
	if (fromDrive != PestoEmotion::Idle) {
		return fromDrive;
	}
	return pestoEmotionFromHead(rx, ry, deadzone);
}

#endif // PESTO_EMOTION_H

// Idle playful bursts inside Sense React — Arduino-free for native tests.
// Hosted Avoid free-roam overlays dance + light follow (see avoid_roam.h).
#ifndef SENSE_PLAYFUL_H

#define SENSE_PLAYFUL_H



#include <cstdint>



constexpr uint32_t SENSE_PLAYFUL_AFTER_MS = 12000;

constexpr uint32_t SENSE_PLAYFUL_BURST_MS = 12000;

constexpr uint32_t SENSE_PLAYFUL_COOLDOWN_MS = 8000;



enum class SensePlayfulMode : uint8_t {

	None = 0,

	Dance = 1,

	Avoid = 2,

	FollowLight = 3,

};



inline SensePlayfulMode sensePlayfulPick(uint32_t seed) {

	switch (seed % 3) {

		case 0:

			return SensePlayfulMode::Dance;

		case 1:

			return SensePlayfulMode::Avoid;

		default:

			return SensePlayfulMode::FollowLight;

	}

}



inline SensePlayfulMode sensePlayfulPickIdle() {

	return SensePlayfulMode::Avoid;

}



inline bool sensePlayfulShouldStart(uint32_t idleMs, bool seeking, bool manual,

		SensePlayfulMode current, uint32_t now, uint32_t nextBurstEarliestMs) {

	if (current != SensePlayfulMode::None) {

		return false;

	}

	if (manual || seeking) {

		return false;

	}

	if (idleMs < SENSE_PLAYFUL_AFTER_MS) {

		return false;

	}

	return now >= nextBurstEarliestMs;

}



inline bool sensePlayfulBurstExpired(uint32_t now, uint32_t burstStartMs) {

	if (burstStartMs == 0) {

		return false;

	}

	return (now - burstStartMs) >= SENSE_PLAYFUL_BURST_MS;

}



#endif // SENSE_PLAYFUL_H


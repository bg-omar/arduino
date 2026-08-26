//
// Non-blocking millisecond timing. Use instead of delay() in loop-path code.
//

#ifndef DEADLINE_H
#define DEADLINE_H

#include <cstdint>

struct Deadline {
	uint32_t due = 0;

	bool isDue(uint32_t now) const {
		return static_cast<int32_t>(now - due) >= 0;
	}

	void after(uint32_t now, uint32_t ms) {
		due = now + ms;
	}
};

inline bool elapsed(uint32_t now, uint32_t& last, uint32_t interval) {
	if (static_cast<uint32_t>(now - last) < interval) {
		return false;
	}
	last = now;
	return true;
}

#endif // DEADLINE_H

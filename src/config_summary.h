//
// Compact OLED helpers: strip USE_ prefix, two flags per line, I2C hex.
// Arduino-free so native tests can cover formatting.
//

#ifndef CONFIG_SUMMARY_H
#define CONFIG_SUMMARY_H

#include <cstddef>
#include <cstdio>
#include <cstring>

inline bool summaryAppend(char* dest, size_t cap, size_t& used, const char* token) {
	if (dest == nullptr || token == nullptr || cap == 0) {
		return false;
	}
	const size_t len = strlen(token);
	const size_t extra = (used == 0) ? 0 : 1;
	if (len == 0 || used + extra + len + 1 > cap) {
		return false;
	}
	if (used > 0) {
		dest[used++] = ' ';
	}
	memcpy(dest + used, token, len);
	used += len;
	dest[used] = '\0';
	return true;
}

inline bool summaryAppendIf(char* dest, size_t cap, size_t& used, const char* token, bool enabled) {
	if (!enabled) {
		return true;
	}
	return summaryAppend(dest, cap, used, token);
}

inline void summaryHexByte(char* dest, unsigned value) {
	static const char kHex[] = "0123456789ABCDEF";
	dest[0] = kHex[(value >> 4) & 0x0F];
	dest[1] = kHex[value & 0x0F];
	dest[2] = '\0';
}

inline const char* configDisplayName(const char* key) {
	if (key == nullptr) {
		return "";
	}
	if (strncmp(key, "USE_", 4) == 0) {
		return key + 4;
	}
	return key;
}

// 41 visible chars, '|' always at column 20:
// [name 17][ ][0/1][ ]|[ ][0/1][ ][name 17]
constexpr int kFlagLineWidth = 41;
constexpr int kFlagNameWidth = 17;
constexpr int kFlagPipeColumn = 20;

inline bool formatFlagPair(
		char* dest,
		size_t cap,
		const char* name1,
		bool value1,
		const char* name2,
		bool hasSecond,
		bool value2) {
	if (dest == nullptr || cap < static_cast<size_t>(kFlagLineWidth + 1) || name1 == nullptr || name1[0] == '\0') {
		return false;
	}
	int written = 0;
	if (hasSecond && name2 != nullptr && name2[0] != '\0') {
		written = snprintf(
				dest,
				cap,
				"%*.*s %d | %d %-*.*s",
				kFlagNameWidth,
				kFlagNameWidth,
				name1,
				value1 ? 1 : 0,
				value2 ? 1 : 0,
				kFlagNameWidth,
				kFlagNameWidth,
				name2);
	} else {
		written = snprintf(
				dest,
				cap,
				"%*.*s %d |",
				kFlagNameWidth,
				kFlagNameWidth,
				name1,
				value1 ? 1 : 0);
	}
	return written > 0 && dest[kFlagPipeColumn] == '|';
}

#endif // CONFIG_SUMMARY_H

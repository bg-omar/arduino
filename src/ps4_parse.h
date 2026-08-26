//
// PS4 Serial1 message parser (no STL).
//

#ifndef PS4_PARSE_H
#define PS4_PARSE_H

#include <cctype>
#include <cstdint>
#include <cstdlib>

inline bool ps4_has_letter(const char* msg) {
	for (const char* p = msg; *p; ++p) {
		if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
			return true;
		}
	}
	return false;
}

inline bool ps4_is_sep(char c) {
	return c == '+' || c == '-' || c == '_';
}

inline int ps4_parse_term(const char* start, const char* end) {
	while (start < end && isspace(static_cast<unsigned char>(*start))) {
		++start;
	}
	while (end > start && !isdigit(static_cast<unsigned char>(*(end - 1)))) {
		--end;
	}
	if (start >= end) {
		return 0;
	}

	char buf[16];
	size_t len = static_cast<size_t>(end - start);
	if (len >= sizeof(buf)) {
		len = sizeof(buf) - 1;
	}
	for (size_t i = 0; i < len; ++i) {
		buf[i] = start[i];
	}
	buf[len] = '\0';
	return atoi(buf);
}

// Parse "1100", "6127+7127", "4000+5000"; up to 2 ints (stick / L2R2).
// Letters in message -> out[0]=0, out[1]=0, count=2 (same as old {0,0} vector).
inline bool parsePs4Message(const char* msg, int* out, uint8_t* count) {
	*count = 0;
	if (msg == nullptr || *msg == '\0') {
		return false;
	}

	if (ps4_has_letter(msg)) {
		out[0] = 0;
		out[1] = 0;
		*count = 2;
		return true;
	}

	const char* prev = msg;
	for (const char* p = msg; ; ++p) {
		if (*p == '\0' || ps4_is_sep(*p)) {
			if (p > prev && *count < 2) {
				out[*count] = ps4_parse_term(prev, p);
				++(*count);
			}
			if (*p == '\0') {
				break;
			}
			prev = p + 1;
		}
	}

	return *count > 0;
}

#endif // PS4_PARSE_H

//
// Menu flag draft/snapshot helpers — Arduino-free for native tests.
//

#ifndef MENU_DRAFT_H
#define MENU_DRAFT_H

#include <cstddef>

inline void menuDraftSnapshot(bool* dest, bool* const* flags, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		dest[i] = *flags[i];
	}
}

inline void menuDraftRestore(bool* const* flags, const bool* backup, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		*flags[i] = backup[i];
	}
}

inline bool menuDraftEquals(bool* const* flags, const bool* backup, size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (*flags[i] != backup[i]) {
			return false;
		}
	}
	return true;
}

#endif // MENU_DRAFT_H

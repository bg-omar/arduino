//
// OLED log ring: append stays on the current line, appendln commits.
// Arduino-free so native tests can cover wrap / ring behaviour.
//

#ifndef LOG_BUFFER_H
#define LOG_BUFFER_H

#include <cstddef>
#include <cstring>

struct LogBuffer {
	static constexpr int kLines = 8;
	static constexpr int kLineLength = 42;

	char lines[kLines][kLineLength];
	int next;
	int col;

	LogBuffer() {
		clear();
	}

	void clear() {
		for (int i = 0; i < kLines; ++i) {
			lines[i][0] = '\0';
		}
		next = 0;
		col = 0;
	}

	void append(const char* text) {
		if (text == nullptr) {
			return;
		}
		for (; *text != '\0'; ++text) {
			if (*text == '\n') {
				commitLine();
				continue;
			}
			if (col >= kLineLength - 1) {
				commitLine();
			}
			lines[next][col++] = *text;
			lines[next][col] = '\0';
		}
	}

	void appendln(const char* text) {
		append(text);
		commitLine();
	}

	void commitLine() {
		next = (next + 1) % kLines;
		col = 0;
		lines[next][0] = '\0';
	}

	const char* visualLine(int visualIndex) const {
		if (visualIndex < 0 || visualIndex >= kLines) {
			return "";
		}
		const int index = (next + 1 + visualIndex) % kLines;
		return lines[index];
	}

	int packedCount() const {
		int count = 0;
		for (int i = 0; i < kLines; ++i) {
			if (visualLine(i)[0] != '\0') {
				++count;
			}
		}
		return count;
	}

	const char* packedLine(int packedIndex) const {
		int seen = 0;
		for (int i = 0; i < kLines; ++i) {
			const char* line = visualLine(i);
			if (line[0] == '\0') {
				continue;
			}
			if (seen == packedIndex) {
				return line;
			}
			++seen;
		}
		return "";
	}
};

#endif // LOG_BUFFER_H

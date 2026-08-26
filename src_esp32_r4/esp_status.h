//
// Arduino-free ESP gateway status / log JSON for native tests and the web API.
//

#ifndef ESP_STATUS_H
#define ESP_STATUS_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

struct EspStatus {
	const char* ip;
	const char* ssid;
	const char* ntp;
	const char* wifi;
	int rssi;
	uint32_t uptime_ms;
	uint32_t heap;
};

inline bool jsonPutChar(char* dest, size_t cap, size_t& used, char c) {
	if (dest == nullptr || used + 1 >= cap) {
		return false;
	}
	dest[used++] = c;
	dest[used] = '\0';
	return true;
}

inline bool jsonPutRaw(char* dest, size_t cap, size_t& used, const char* text) {
	if (text == nullptr) {
		return true;
	}
	for (; *text != '\0'; ++text) {
		if (!jsonPutChar(dest, cap, used, *text)) {
			return false;
		}
	}
	return true;
}

inline bool jsonPutQuoted(char* dest, size_t cap, size_t& used, const char* text) {
	if (!jsonPutChar(dest, cap, used, '"')) {
		return false;
	}
	if (text == nullptr) {
		text = "";
	}
	for (; *text != '\0'; ++text) {
		const unsigned char c = static_cast<unsigned char>(*text);
		if (c == '"' || c == '\\') {
			if (!jsonPutChar(dest, cap, used, '\\') || !jsonPutChar(dest, cap, used, static_cast<char>(c))) {
				return false;
			}
		} else if (c < 0x20) {
			if (!jsonPutChar(dest, cap, used, ' ')) {
				return false;
			}
		} else {
			if (!jsonPutChar(dest, cap, used, static_cast<char>(c))) {
				return false;
			}
		}
	}
	return jsonPutChar(dest, cap, used, '"');
}

inline bool formatEspStatusJson(char* dest, size_t cap, const EspStatus& status) {
	if (dest == nullptr || cap < 3) {
		return false;
	}
	size_t used = 0;
	dest[0] = '\0';

	char rssiBuf[16];
	char uptimeBuf[16];
	char heapBuf[16];
	snprintf(rssiBuf, sizeof(rssiBuf), "%d", status.rssi);
	snprintf(uptimeBuf, sizeof(uptimeBuf), "%lu", static_cast<unsigned long>(status.uptime_ms));
	snprintf(heapBuf, sizeof(heapBuf), "%lu", static_cast<unsigned long>(status.heap));

	return jsonPutChar(dest, cap, used, '{')
		&& jsonPutRaw(dest, cap, used, "\"ip\":")
		&& jsonPutQuoted(dest, cap, used, status.ip)
		&& jsonPutRaw(dest, cap, used, ",\"ssid\":")
		&& jsonPutQuoted(dest, cap, used, status.ssid)
		&& jsonPutRaw(dest, cap, used, ",\"ntp\":")
		&& jsonPutQuoted(dest, cap, used, status.ntp)
		&& jsonPutRaw(dest, cap, used, ",\"wifi\":")
		&& jsonPutQuoted(dest, cap, used, status.wifi)
		&& jsonPutRaw(dest, cap, used, ",\"rssi\":")
		&& jsonPutRaw(dest, cap, used, rssiBuf)
		&& jsonPutRaw(dest, cap, used, ",\"uptime_ms\":")
		&& jsonPutRaw(dest, cap, used, uptimeBuf)
		&& jsonPutRaw(dest, cap, used, ",\"heap\":")
		&& jsonPutRaw(dest, cap, used, heapBuf)
		&& jsonPutChar(dest, cap, used, '}');
}

inline bool formatEspLogJson(char* dest, size_t cap, const char* const* lines, int count) {
	if (dest == nullptr || cap < 3 || (count > 0 && lines == nullptr)) {
		return false;
	}
	if (count < 0) {
		count = 0;
	}

	size_t used = 0;
	dest[0] = '\0';
	if (!jsonPutRaw(dest, cap, used, "{\"lines\":[")) {
		return false;
	}
	for (int i = 0; i < count; ++i) {
		if (i > 0 && !jsonPutChar(dest, cap, used, ',')) {
			return false;
		}
		if (!jsonPutQuoted(dest, cap, used, lines[i])) {
			return false;
		}
	}
	return jsonPutRaw(dest, cap, used, "]}");
}

#endif // ESP_STATUS_H

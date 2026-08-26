#include "real_time.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <cstring>

namespace {
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org", 7200, 60000);
bool ntpStarted = false;
}

void real_time::begin() {
	if (WiFi.status() != WL_CONNECTED) {
		ntpStarted = false;
		return;
	}
	timeClient.begin();
	timeClient.update();
	ntpStarted = true;
}

void real_time::poll() {
	if (WiFi.status() != WL_CONNECTED) {
		return;
	}
	if (!ntpStarted) {
		begin();
		return;
	}
	timeClient.update();
}

bool real_time::synced() {
	return ntpStarted && timeClient.isTimeSet();
}

void real_time::format(char* dest, size_t cap) {
	if (dest == nullptr || cap == 0) {
		return;
	}
	if (!synced()) {
		strncpy(dest, "unsynced", cap - 1);
		dest[cap - 1] = '\0';
		return;
	}
	strncpy(dest, timeClient.getFormattedTime().c_str(), cap - 1);
	dest[cap - 1] = '\0';
}

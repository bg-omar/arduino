/*
 * UNO R4 WiFi onboard ESP32-S3 gateway:
 * WiFi STA + status/log web UI. CDC/CMSIS-DAP bridge stays active.
 * PS4-CAM remains on Serial1 of the Renesas chip — this firmware does not host a pad.
 */
 
/*
UNO BLE -->     DC:54:75:C3:D9:EC   -
PC USB Dongel   00:1F:E2:C8:82:BA
ESP 1           66:CB:3E:E9:02:8A
ESP             3c:e9:0e:89:80:84
ESP small cam   3C:E9:0E:88:65:16
PS4 Controller: A4:AE:11:E1:8B:B3 (SONYWA) GooglyEyes
PS5 Controller: 88:03:4C:B5:00:66
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_uno_r4.h>
#include <cstdio>
#include <cstring>

#include "esp_status.h"
#include "log_buffer.h"
#include "real_time.h"
#include "secrets.h"

namespace {
WebServer server(80);
LogBuffer gatewayLog;
uint32_t lastWifiRetryMs = 0;
uint32_t lastNtpPollMs = 0;
bool wifiWasConnected = false;

constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kNtpPollMs = 1000;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;

const char kIndexHtml[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wall-Z ESP gateway</title>
<style>
body{font-family:sans-serif;margin:1.2rem;background:#111;color:#eee}
h1{font-size:1.2rem}
dl{display:grid;grid-template-columns:8rem 1fr;gap:.25rem 1rem}
dt{opacity:.7}
pre{background:#222;padding:.75rem;min-height:8rem;white-space:pre-wrap}
</style>
</head>
<body>
<h1>Wall-Z ESP32-S3 gateway</h1>
<dl>
<dt>wifi</dt><dd id="wifi">-</dd>
<dt>ip</dt><dd id="ip">-</dd>
<dt>ssid</dt><dd id="ssid">-</dd>
<dt>rssi</dt><dd id="rssi">-</dd>
<dt>uptime</dt><dd id="uptime">-</dd>
<dt>heap</dt><dd id="heap">-</dd>
<dt>ntp</dt><dd id="ntp">-</dd>
</dl>
<h2>log</h2>
<pre id="log"></pre>
<script>
async function tick(){
  try{
    const s=await (await fetch('/api/status')).json();
    wifi.textContent=s.wifi;
    ip.textContent=s.ip;
    ssid.textContent=s.ssid;
    rssi.textContent=s.rssi;
    uptime.textContent=s.uptime_ms;
    heap.textContent=s.heap;
    ntp.textContent=s.ntp;
    const log=await (await fetch('/api/log')).json();
    document.getElementById('log').textContent=(log.lines||[]).join('\n');
  }catch(e){}
}
tick();
setInterval(tick,500);
</script>
</body>
</html>
)HTML";

void logLine(const char* text) {
	gatewayLog.appendln(text);
	Serial.println(text);
}

bool wifiConnected() {
	return WiFi.status() == WL_CONNECTED;
}

void printIpToLog() {
	char line[LogBuffer::kLineLength];
	snprintf(line, sizeof(line), "IP %s", WiFi.localIP().toString().c_str());
	logLine(line);
}

void connectWifi(uint32_t timeoutMs) {
	if (wifiConnected()) {
		return;
	}
	logLine("WiFi connecting");
	WiFi.mode(WIFI_STA);
	WiFi.begin(SECRET_SSID, SECRET_PASS);
	const uint32_t start = millis();
	while (!wifiConnected() && (millis() - start) < timeoutMs) {
		delay(250);
		Serial.print('.');
	}
	Serial.println();
	if (wifiConnected()) {
		logLine("WiFi up");
		printIpToLog();
		wifiWasConnected = true;
		real_time::begin();
	} else {
		logLine("WiFi fail");
		wifiWasConnected = false;
	}
}

void copyBounded(char* dest, size_t cap, const char* src) {
	if (dest == nullptr || cap == 0) {
		return;
	}
	if (src == nullptr) {
		dest[0] = '\0';
		return;
	}
	strncpy(dest, src, cap - 1);
	dest[cap - 1] = '\0';
}

void fillStatus(EspStatus& status, char* ipBuf, size_t ipCap, char* ssidBuf, size_t ssidCap, char* ntpBuf, size_t ntpCap) {
	if (wifiConnected()) {
		copyBounded(ipBuf, ipCap, WiFi.localIP().toString().c_str());
		copyBounded(ssidBuf, ssidCap, WiFi.SSID().c_str());
		status.wifi = "connected";
		status.rssi = WiFi.RSSI();
	} else {
		copyBounded(ipBuf, ipCap, "0.0.0.0");
		copyBounded(ssidBuf, ssidCap, SECRET_SSID);
		status.wifi = "disconnected";
		status.rssi = 0;
	}
	status.ip = ipBuf;
	status.ssid = ssidBuf;
	real_time::format(ntpBuf, ntpCap);
	status.ntp = ntpBuf;
	status.uptime_ms = millis();
	status.heap = ESP.getFreeHeap();
}

void handleRoot() {
	server.send_P(200, "text/html", kIndexHtml);
}

void handleStatus() {
	char ipBuf[20];
	char ssidBuf[33];
	char ntpBuf[16];
	char json[384];
	EspStatus status;
	fillStatus(status, ipBuf, sizeof(ipBuf), ssidBuf, sizeof(ssidBuf), ntpBuf, sizeof(ntpBuf));
	if (!formatEspStatusJson(json, sizeof(json), status)) {
		server.send(500, "application/json", "{\"error\":\"status\"}");
		return;
	}
	server.send(200, "application/json", json);
}

void handleLog() {
	const char* lines[LogBuffer::kLines];
	const int count = gatewayLog.packedCount();
	for (int i = 0; i < count; ++i) {
		lines[i] = gatewayLog.packedLine(i);
	}
	char json[512];
	if (!formatEspLogJson(json, sizeof(json), lines, count)) {
		server.send(500, "application/json", "{\"error\":\"log\"}");
		return;
	}
	server.send(200, "application/json", json);
}

void handleNotFound() {
	server.send(404, "text/plain", "not found");
}
}  // namespace

void setup() {
	esp_uno_r4_setup();
	Serial.begin(115200);
	delay(50);
	logLine("Wall-Z ESP boot");

	connectWifi(kWifiConnectTimeoutMs);

	server.on("/", HTTP_GET, handleRoot);
	server.on("/api/status", HTTP_GET, handleStatus);
	server.on("/api/log", HTTP_GET, handleLog);
	server.onNotFound(handleNotFound);
	server.begin();
	logLine("HTTP :80");
	if (wifiConnected()) {
		Serial.print("Open http://");
		Serial.println(WiFi.localIP());
	}
}

void loop() {
	server.handleClient();

	const uint32_t now = millis();
	const bool nowUp = wifiConnected();
	if (nowUp && !wifiWasConnected) {
		logLine("WiFi up");
		printIpToLog();
		real_time::begin();
	}
	wifiWasConnected = nowUp;

	if (!nowUp && (now - lastWifiRetryMs) >= kWifiRetryMs) {
		lastWifiRetryMs = now;
		logLine("WiFi retry");
		WiFi.disconnect();
		WiFi.mode(WIFI_STA);
		WiFi.begin(SECRET_SSID, SECRET_PASS);
	}
	if ((now - lastNtpPollMs) >= kNtpPollMs) {
		lastNtpPollMs = now;
		real_time::poll();
	}
}

#include <cstring>
#include <unity.h>
#include "esp_status.h"
#include "log_buffer.h"

void test_status_json_contains_core_fields() {
	EspStatus status;
	status.ip = "192.168.1.42";
	status.ssid = "KT2";
	status.ntp = "20:55:01";
	status.wifi = "connected";
	status.rssi = -51;
	status.uptime_ms = 12345;
	status.heap = 180000;

	char json[256];
	TEST_ASSERT_TRUE(formatEspStatusJson(json, sizeof(json), status));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"ip\":\"192.168.1.42\""));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"ssid\":\"KT2\""));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"ntp\":\"20:55:01\""));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"wifi\":\"connected\""));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"rssi\":-51"));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"uptime_ms\":12345"));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"heap\":180000"));
}

void test_status_json_escapes_quotes() {
	EspStatus status;
	status.ip = "0.0.0.0";
	status.ssid = "net\"work";
	status.ntp = "unsynced";
	status.wifi = "disconnected";
	status.rssi = 0;
	status.uptime_ms = 0;
	status.heap = 1;

	char json[128];
	TEST_ASSERT_TRUE(formatEspStatusJson(json, sizeof(json), status));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"ssid\":\"net\\\"work\""));
}

void test_status_json_null_strings_become_empty() {
	EspStatus status;
	status.ip = nullptr;
	status.ssid = nullptr;
	status.ntp = nullptr;
	status.wifi = nullptr;
	status.rssi = 0;
	status.uptime_ms = 0;
	status.heap = 0;

	char json[128];
	TEST_ASSERT_TRUE(formatEspStatusJson(json, sizeof(json), status));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"ip\":\"\""));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"ssid\":\"\""));
}

void test_status_json_truncates() {
	EspStatus status;
	status.ip = "10.0.0.1";
	status.ssid = "home";
	status.ntp = "12:00:00";
	status.wifi = "connected";
	status.rssi = -20;
	status.uptime_ms = 9;
	status.heap = 8;

	char json[16];
	TEST_ASSERT_FALSE(formatEspStatusJson(json, sizeof(json), status));
}

void test_log_json_empty_array() {
	char json[32];
	TEST_ASSERT_TRUE(formatEspLogJson(json, sizeof(json), nullptr, 0));
	TEST_ASSERT_EQUAL_STRING("{\"lines\":[]}", json);
}

void test_log_json_two_lines() {
	const char* lines[] = {"boot", "wifi ok"};
	char json[64];
	TEST_ASSERT_TRUE(formatEspLogJson(json, sizeof(json), lines, 2));
	TEST_ASSERT_EQUAL_STRING("{\"lines\":[\"boot\",\"wifi ok\"]}", json);
}

void test_log_json_from_log_buffer_packed_lines() {
	LogBuffer buf;
	buf.appendln("Wall-Z ESP boot");
	buf.appendln("WiFi up");

	const char* lines[8];
	const int count = buf.packedCount();
	for (int i = 0; i < count; ++i) {
		lines[i] = buf.packedLine(i);
	}

	char json[96];
	TEST_ASSERT_TRUE(formatEspLogJson(json, sizeof(json), lines, count));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"Wall-Z ESP boot\""));
	TEST_ASSERT_NOT_NULL(strstr(json, "\"WiFi up\""));
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_status_json_contains_core_fields);
	RUN_TEST(test_status_json_escapes_quotes);
	RUN_TEST(test_status_json_null_strings_become_empty);
	RUN_TEST(test_status_json_truncates);
	RUN_TEST(test_log_json_empty_array);
	RUN_TEST(test_log_json_two_lines);
	RUN_TEST(test_log_json_from_log_buffer_packed_lines);
	return UNITY_END();
}

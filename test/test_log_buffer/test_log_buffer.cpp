#include <cstdint>
#include <cstring>
#include <unity.h>
#include "log_buffer.h"
#include "config_summary.h"

void test_append_stays_on_same_line() {
	LogBuffer buf;
	buf.append("Serial");
	buf.append("1 & ");
	buf.append("AT");
	TEST_ASSERT_EQUAL_INT(1, buf.packedCount());
	TEST_ASSERT_EQUAL_STRING("Serial1 & AT", buf.packedLine(0));
}

void test_appendln_commits_new_line() {
	LogBuffer buf;
	buf.append("UART ");
	buf.appendln("9600");
	buf.appendln("SD ok");
	TEST_ASSERT_EQUAL_INT(2, buf.packedCount());
	TEST_ASSERT_EQUAL_STRING("UART 9600", buf.packedLine(0));
	TEST_ASSERT_EQUAL_STRING("SD ok", buf.packedLine(1));
}

void test_embedded_newline_splits() {
	LogBuffer buf;
	buf.appendln("boot\nok");
	TEST_ASSERT_EQUAL_INT(2, buf.packedCount());
	TEST_ASSERT_EQUAL_STRING("boot", buf.packedLine(0));
	TEST_ASSERT_EQUAL_STRING("ok", buf.packedLine(1));
}

void test_wraps_at_line_length() {
	LogBuffer buf;
	char longLine[LogBuffer::kLineLength + 8];
	for (int i = 0; i < LogBuffer::kLineLength + 4; ++i) {
		longLine[i] = 'A' + (i % 26);
	}
	longLine[LogBuffer::kLineLength + 4] = '\0';
	buf.appendln(longLine);
	TEST_ASSERT_EQUAL_INT(2, buf.packedCount());
	TEST_ASSERT_EQUAL_INT(LogBuffer::kLineLength - 1, static_cast<int>(strlen(buf.packedLine(0))));
	TEST_ASSERT_EQUAL_INT(5, static_cast<int>(strlen(buf.packedLine(1))));
}

void test_ring_drops_oldest() {
	LogBuffer buf;
	for (int i = 0; i < LogBuffer::kLines + 2; ++i) {
		char token[4];
		token[0] = static_cast<char>('A' + i);
		token[1] = '\0';
		buf.appendln(token);
	}
	TEST_ASSERT_EQUAL_INT(LogBuffer::kLines - 1, buf.packedCount());
	TEST_ASSERT_EQUAL_STRING("D", buf.packedLine(0));
}

void test_packed_skips_empty_and_starts_at_top() {
	LogBuffer buf;
	buf.appendln("first");
	TEST_ASSERT_EQUAL_INT(1, buf.packedCount());
	TEST_ASSERT_EQUAL_STRING("first", buf.packedLine(0));
	TEST_ASSERT_EQUAL_STRING("", buf.packedLine(1));
}

void test_summary_append_joins_with_space() {
	char line[16];
	size_t used = 0;
	line[0] = '\0';
	TEST_ASSERT_TRUE(summaryAppend(line, sizeof(line), used, "PS4"));
	TEST_ASSERT_TRUE(summaryAppend(line, sizeof(line), used, "GYRO"));
	TEST_ASSERT_EQUAL_STRING("PS4 GYRO", line);
	TEST_ASSERT_EQUAL_UINT32(8u, static_cast<uint32_t>(used));
}

void test_summary_append_if_skips_disabled() {
	char line[16];
	size_t used = 0;
	line[0] = '\0';
	TEST_ASSERT_TRUE(summaryAppendIf(line, sizeof(line), used, "PS4", true));
	TEST_ASSERT_TRUE(summaryAppendIf(line, sizeof(line), used, "LCD", false));
	TEST_ASSERT_TRUE(summaryAppendIf(line, sizeof(line), used, "BARO", true));
	TEST_ASSERT_EQUAL_STRING("PS4 BARO", line);
}

void test_summary_append_overflow() {
	char line[8];
	size_t used = 0;
	line[0] = '\0';
	TEST_ASSERT_TRUE(summaryAppend(line, sizeof(line), used, "PS4"));
	TEST_ASSERT_FALSE(summaryAppend(line, sizeof(line), used, "GYRO"));
	TEST_ASSERT_EQUAL_STRING("PS4", line);
}

void test_summary_hex_byte() {
	char hex[3];
	summaryHexByte(hex, 0x3C);
	TEST_ASSERT_EQUAL_STRING("3C", hex);
	summaryHexByte(hex, 0x0A);
	TEST_ASSERT_EQUAL_STRING("0A", hex);
}

void test_config_display_name_strips_use_prefix() {
	TEST_ASSERT_EQUAL_STRING("ADAFRUIT", configDisplayName("USE_ADAFRUIT"));
	TEST_ASSERT_EQUAL_STRING("U8G2", configDisplayName("USE_U8G2"));
	TEST_ASSERT_EQUAL_STRING("SMALL", configDisplayName("SMALL"));
	TEST_ASSERT_EQUAL_STRING("LOG_DEBUG", configDisplayName("LOG_DEBUG"));
	TEST_ASSERT_EQUAL_STRING("DISPLAY_DEMO", configDisplayName("DISPLAY_DEMO"));
}

void test_format_flag_pair_two_values() {
	char line[42];
	TEST_ASSERT_TRUE(formatFlagPair(line, sizeof(line), "ADAFRUIT", true, "U8G2", true, false));
	TEST_ASSERT_EQUAL_INT('|', line[kFlagPipeColumn]);
	TEST_ASSERT_EQUAL_STRING("         ADAFRUIT 1 | 0 U8G2             ", line);
}

void test_format_flag_pair_pipe_stays_centered() {
	char shortLine[42];
	char longLine[42];
	TEST_ASSERT_TRUE(formatFlagPair(shortLine, sizeof(shortLine), "DOT", true, "SW", true, false));
	TEST_ASSERT_TRUE(formatFlagPair(longLine, sizeof(longLine), "MATRIX_PREVIEW", false, "READ_ESP32", true, true));
	TEST_ASSERT_EQUAL_INT('|', shortLine[kFlagPipeColumn]);
	TEST_ASSERT_EQUAL_INT('|', longLine[kFlagPipeColumn]);
	TEST_ASSERT_EQUAL_INT(kFlagLineWidth, static_cast<int>(strlen(longLine)));
}

void test_format_flag_pair_odd_last() {
	char line[42];
	TEST_ASSERT_TRUE(formatFlagPair(line, sizeof(line), "LCD", false, "", false, false));
	TEST_ASSERT_EQUAL_INT('|', line[kFlagPipeColumn]);
	TEST_ASSERT_EQUAL_STRING("              LCD 0 |", line);
}

void setUp() {}
void tearDown() {}

int main(int argc, char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_append_stays_on_same_line);
	RUN_TEST(test_appendln_commits_new_line);
	RUN_TEST(test_embedded_newline_splits);
	RUN_TEST(test_wraps_at_line_length);
	RUN_TEST(test_ring_drops_oldest);
	RUN_TEST(test_packed_skips_empty_and_starts_at_top);
	RUN_TEST(test_summary_append_joins_with_space);
	RUN_TEST(test_summary_append_if_skips_disabled);
	RUN_TEST(test_summary_append_overflow);
	RUN_TEST(test_summary_hex_byte);
	RUN_TEST(test_config_display_name_strips_use_prefix);
	RUN_TEST(test_format_flag_pair_two_values);
	RUN_TEST(test_format_flag_pair_pipe_stays_centered);
	RUN_TEST(test_format_flag_pair_odd_last);
	return UNITY_END();
}

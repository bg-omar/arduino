//
// Created by mr on 11/20/2023.
//

#include "logger.h"
#include "I2Cscanner.h"
#include "config_summary.h"
#include "log_buffer.h"
#include <Wire.h>

void I2Cscanner::scan() {
	char line[LogBuffer::kLineLength];
	size_t used = 0;
	int nDevices = 0;
	line[0] = '\0';
	summaryAppend(line, sizeof(line), used, "I2C");

	for (byte address = 1; address < 127; address++) {
		Wire.beginTransmission(address);
		const byte error = Wire.endTransmission();

		if (error == 0) {
			char hex[3];
			summaryHexByte(hex, address);
			if (!summaryAppend(line, sizeof(line), used, hex)) {
				logger::logln(line);
				used = 0;
				line[0] = '\0';
				summaryAppend(line, sizeof(line), used, hex);
			}
			nDevices++;
		} else if (error == 4) {
			char hex[3];
			summaryHexByte(hex, address);
			logger::log("I2C err ");
			logger::logln(hex);
		}
	}

	if (nDevices == 0) {
		logger::logln("I2C none");
	} else {
		logger::logln(line);
	}
}

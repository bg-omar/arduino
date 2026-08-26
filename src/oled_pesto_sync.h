// Downsample 128x64 OLED face bitmap to Pesto 16x8 column-major layout.
#ifndef OLED_PESTO_SYNC_H
#define OLED_PESTO_SYNC_H

#include <cstdint>

inline bool oledBitmapPixelOn(const uint8_t* oled1024, int x, int y) {
	const int byteIdx = y * 16 + x / 8;
	const int bit = 7 - (x % 8);
	return (oled1024[byteIdx] & static_cast<uint8_t>(1u << bit)) != 0;
}

inline void oledBitmapDownsample16x8(const uint8_t* oled1024, uint8_t out16[16]) {
	for (int col = 0; col < 16; ++col) {
		uint8_t colByte = 0;
		for (int row = 0; row < 8; ++row) {
			int white = 0;
			for (int dy = 0; dy < 8; ++dy) {
				for (int dx = 0; dx < 8; ++dx) {
					const int x = col * 8 + dx;
					const int y = row * 8 + dy;
					if (oledBitmapPixelOn(oled1024, x, y)) {
						++white;
					}
				}
			}
			if (white >= 32) {
				colByte |= static_cast<uint8_t>(1u << row);
			}
		}
		out16[col] = colByte;
	}
}

#endif // OLED_PESTO_SYNC_H

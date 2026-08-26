#ifndef MIC_PEAK_H
#define MIC_PEAK_H

#include <cstdint>

struct MicPeakState {
	uint16_t maxL = 0;
	uint16_t minL = 1024;
	uint16_t maxR = 0;
	uint16_t minR = 1024;
};

inline void micPeakReset(MicPeakState& s) {
	s.maxL = 0;
	s.minL = 1024;
	s.maxR = 0;
	s.minR = 1024;
}

inline void micPeakSample(MicPeakState& s, uint16_t sampleL, uint16_t sampleR) {
	if (sampleL < 1024) {
		if (sampleL > s.maxL) {
			s.maxL = sampleL;
		}
		if (sampleL < s.minL) {
			s.minL = sampleL;
		}
	}
	if (sampleR < 1024) {
		if (sampleR > s.maxR) {
			s.maxR = sampleR;
		}
		if (sampleR < s.minR) {
			s.minR = sampleR;
		}
	}
}

inline uint16_t micPeakToPeak(uint16_t maxV, uint16_t minV) {
	return static_cast<uint16_t>(maxV - minV);
}

#endif // MIC_PEAK_H

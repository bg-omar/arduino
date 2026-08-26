// OLED face clip player — metadata + frame advance (Arduino-free).
#ifndef OLED_FACE_CLIP_H
#define OLED_FACE_CLIP_H

#include <cstdint>

#include "pesto_emotion.h"
#include "sense_mood.h"

enum class OledFaceClipId : uint8_t {
	IdleBlink = 0,
	LookLR,
	Alert,
	Happy,
};

struct OledFaceClipMeta {
	uint8_t frameCount;
	uint16_t frameIntervalMs;
};

inline OledFaceClipMeta oledFaceClipMeta(OledFaceClipId id) {
	switch (id) {
		case OledFaceClipId::IdleBlink:
			return OledFaceClipMeta{12, 125};
		case OledFaceClipId::LookLR:
			return OledFaceClipMeta{8, 250};
		case OledFaceClipId::Alert:
			return OledFaceClipMeta{16, 125};
		case OledFaceClipId::Happy:
			return OledFaceClipMeta{20, 100};
		default:
			return OledFaceClipMeta{1, 125};
	}
}

inline uint8_t oledFaceAdvanceFrame(OledFaceClipId clip, uint8_t frameIdx, uint32_t now,
		uint32_t lastMs) {
	const OledFaceClipMeta meta = oledFaceClipMeta(clip);
	if (meta.frameCount == 0) {
		return 0;
	}
	if (lastMs == 0 || (now - lastMs) >= meta.frameIntervalMs) {
		return (uint8_t)((frameIdx + 1) % meta.frameCount);
	}
	return frameIdx;
}

inline uint8_t oledFaceDisplayFrame(OledFaceClipId clip, uint8_t frameIdx, PestoEmotion emotion) {
	if (clip != OledFaceClipId::LookLR) {
		return frameIdx;
	}
	const OledFaceClipMeta meta = oledFaceClipMeta(clip);
	const uint8_t half = meta.frameCount / 2;
	if (half == 0) {
		return frameIdx;
	}
	if (emotion == PestoEmotion::LookLeft) {
		return (uint8_t)(frameIdx % half);
	}
	if (emotion == PestoEmotion::LookRight) {
		return (uint8_t)(half + (frameIdx % half));
	}
	return frameIdx;
}

inline OledFaceClipId oledFaceForSense(PestoEmotion emotion, SenseTrigger trigger, bool seeking) {
	if (trigger == SenseTrigger::Distance || trigger == SenseTrigger::Gyro) {
		return OledFaceClipId::Alert;
	}
	if (trigger == SenseTrigger::Mic || trigger == SenseTrigger::Light) {
		if (emotion == PestoEmotion::LookLeft || emotion == PestoEmotion::LookRight) {
			return OledFaceClipId::LookLR;
		}
		return OledFaceClipId::Happy;
	}
	if (trigger == SenseTrigger::Baro) {
		return OledFaceClipId::Happy;
	}
	if (seeking) {
		return OledFaceClipId::Happy;
	}
	(void)emotion;
	return OledFaceClipId::IdleBlink;
}

#ifdef ARDUINO
#include <Arduino.h>
const uint8_t* oledFaceFrameData(OledFaceClipId id, uint8_t frameIdx);
#endif

#endif // OLED_FACE_CLIP_H

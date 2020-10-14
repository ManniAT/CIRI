#pragma once

class Beeper {
private:
	bool _IsInited = false;
	uint8_t _Channel = 1;
	uint8_t _Pin = 21;
	const int freq = 2000;
	const int resolution = 8;
	const int ToneLow = 1555;
	const int ToneMediume = 2222;
	const int ToneDefault = 4444;
	const int ToneHigh = 7777;
	bool IsMuted = false;
	void Init() {
		ledcSetup(_Channel, freq, resolution);
		ledcAttachPin(_Pin, _Channel);
		ledcWrite(_Channel, 255);
		ledcWriteTone(_Channel, 0);
	}
public:
	Beeper(uint8_t pChannel, uint8_t pPin) {
		_Channel = pChannel;
		_Pin = pPin;

	}
	void ToneOff() {
		if (!_IsInited) {
			Init();
		}
	}

};
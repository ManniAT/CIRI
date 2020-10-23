#pragma once

class Tone {
public:
	enum Value : int16_t {
		LOW_F = 1555, MEDIUM_F = 2222, DEFAULT_F = 4444, HIGH_F = 7777
	};
	Tone() : Tone(Tone::DEFAULT_F) {}
	Tone(Value pTone) : _Value(pTone) {}
	operator int16_t() const { return((int16_t)_Value); }
	operator double() const { return((double)_Value); }
private:
	Value _Value;
};
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
public:
	void Init() {
		ledcSetup(_Channel, freq, resolution);
		ledcAttachPin(_Pin, _Channel);
		ledcWrite(_Channel, 255);
		ledcWriteTone(_Channel, 0);
		_IsInited = true;
	}

	bool IsMuted = false;
	Beeper(uint8_t pChannel, uint8_t pPin) {
		_Channel = pChannel;
		_Pin = pPin;
	}
	bool StartTone(Tone pTone,bool pIgnoreMute=false) {
		if (IsMuted && !pIgnoreMute) {
			return(false);
		}
		if (!_IsInited) {
			Init();
		}
		ledcWriteTone(_Channel, pTone);
		return(true);
	}
	void Beep(Tone pTone, byte pDurationInMilliseconds, bool pIgnoreMute=false) {
		if (StartTone(pTone, pIgnoreMute)) {
			delay(pDurationInMilliseconds);
			ledcWriteTone(_Channel, 0);
		}
	}
	void PlayKeyTone() {
		Beep(Tone::LOW_F, 300, true);
	}
	void ToneOff() {
		if (!_IsInited) {
			Init();
		}
		ledcWriteTone(_Channel, 0);
	}

};
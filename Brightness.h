#pragma once
#define MAXINDEX(a) ((sizeof(a) / sizeof(*a))-1)
class Brightness {

public:
	enum  Value : byte {
		FULL = 255,
		NORM_10 = 255, NORM_9 = 200, NORM_8 = 180, NORM_7 = 130, NORM_6 = 100, NORM_5 = 80, NORM_4 = 60, NORM_3 = 50, NORM_2 = 40, NORM_1 = 30,
		LOW_10 = 24, LOW_9 = 18, LOW_8 = 13, LOW_7 = 9, LOW_6 = 7,
		LOW_5 = 5, LOW_4 = 4, LOW_3 = 3, LOW_2 = 2, LOW_1 = 1,
		OFF = 0
	};
	Brightness() : Brightness(Brightness::OFF) {}
	Brightness(Value pBright) : _Value(pBright) { }
	Brightness(int pNumZeroBased) : _Value(GetBrightness(pNumZeroBased)) {	}
private:
	Value GetBrightness(int pNum) {
		if (pNum < 0) {
			pNum == 0;
		}
		else if (pNum > MAXINDEX(DisplayBrights)) {
			pNum = MAXINDEX(DisplayBrights);
		}
		return(DisplayBrights[pNum]);
	}
	Value _Value;
	static const Value DisplayBrights[21];
public:
	bool operator==(Brightness a) { return _Value == a._Value; }
	bool operator==(Value a) { return _Value == a; }
	bool operator!=(Brightness a) { return _Value != a._Value; }
	Brightness& operator++(int) {
		if (_Value < FULL) {
			for (int nX = 0; nX < NumBrights-1; nX++) {
				if (_Value == DisplayBrights[nX]) {
					_Value = DisplayBrights[nX + 1]; 
					break;
				}
			}
		}
		return(*this);
	}
	Brightness& operator--(int) {
		if (_Value > OFF) {
			for (int nX = 1; nX < NumBrights; nX++) {
				if (_Value == DisplayBrights[nX]) {
					_Value = DisplayBrights[nX - 1]; break;
				}
			}
		}
		return(*this);
	}
	operator byte() const { return((byte)_Value); }
	static const int NumBrights = MAXINDEX(DisplayBrights)+1;
};
const Brightness::Value Brightness::DisplayBrights[] = {
OFF,
LOW_1, LOW_2, LOW_3, LOW_4, LOW_5, LOW_6, LOW_7, LOW_8, LOW_9, LOW_10,
NORM_1, NORM_2, NORM_3, NORM_4, NORM_5, NORM_6, NORM_7, NORM_8, NORM_9, FULL };

#pragma once

class EE895_DeltaReading {
private:
	float _OldTempC;
	float _OldTempF;
	float _OldTempK;
	short _OldCO2;
	short _OldCO2Raw;
	float _OldPressureMBar;
	float _OldPressurePSI;
public:
	float DeltaTempC = 0.5;
	float DeltaTempF = 0.5;
	float DeltaTempK = 0.5;
	short DeltaCO2 = 2;
	short DeltaCO2Raw = 5;
	float DeltaPressureMBar = 0.5;
	float DeltaPressurePSI = 0.5;

	float TempC;
	float TempF;
	float TempK;
	short CO2;
	short CO2Raw;
	float PressureMBar;
	float PressurePSI;

};

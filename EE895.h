#pragma once
#include <Arduino.h>
#include <ModbusMaster.h>

/*
INTEGER 16 bit
Parameter 			Unit 	Scale	Reg.Nr [DEC]	Reg.Addr. [HEX]
Temperature T** 	°C 		100 	4002				0xFA1
Temperature T** 	°F 		50 		4003				0xFA2
Temperature T** 	K 		50		4005				0xFA4
CO2 (average) 		ppm 	1		4031				0xFBE
CO2 (raw) 			ppm 	1		4032				0xFBF
Pressure p* 		mbar	10		4101				0x1004
Pressure p* 		psi 	100		4102				0x1005
*/

#define READ_TEMP_AND_CO2_IN_ON_CALL

#define NUMWORDSTempAndCO2 31

#define ADDRTemperature 0xFA1
#define NUMWORDSTemperature 3
#define ADDRCO2 0xFBE
#define NUMWORDSCO2 2
#define ADDRPressure 0x1004
#define NUMWORDSPressure 2
#define ADDRCO2Interval 0x1450
const uint8_t SLAVE_ID = 0x5F;

class EE895_Reading {
private:
	const char _JSONTemplate[120] = "{\"TempC\": %0.1f, \"TempF\": %0.1f, \"TempK\": %0.1f, \"CO2\": %d, \"CO2Raw\": %d, \"PMbar\": %0.1f, \"PPSI\": %0.1f}";
public:
	bool Ignore_TempC = false;
	bool Ignore_TempF = false;
	bool Ignore_TempK = false;
	bool Ignore_CO2 = false;
	bool Ignore_CO2Raw = false;
	bool Ignore_PressureMBar = false;
	bool Ignore_PressurePSI = false;

	bool TempC_Changed;
	bool TempF_Changed;
	bool TempK_Changed;
	bool CO2_Changed;
	bool CO2Raw_Changed;
	bool PressureMBar_Changed;
	bool PressurePSI_Changed;

	float TempC;
	float TempF;
	float TempK;
	short CO2;
	short CO2Raw;
	float PressureMBar;
	float PressurePSI;
	/// <summary>
	/// Gets the current values in JSON.
	/// </summary>
	/// <param name="pBufferMin130Long">The buffer min130 long.</param>
	void GetJSON(char* pBufferMin130Long) { sprintf(pBufferMin130Long, _JSONTemplate, TempC, TempF, TempK, CO2, CO2Raw, PressureMBar, PressurePSI); }
};


/// <summary>
/// Simple class to provide a EE895 representation
/// Implements the <see cref="EE895_Reading" /> as internal buffer
/// The host creates an instance and periodically calls <see <see cref="ReadData"/> to obtain the data from the sensor.
/// </summary>
class EE895 : public EE895_Reading
{
private:
#ifdef READ_TEMP_AND_CO2_IN_ON_CALL
	uint16_t _TempPlusCO2Buffer[NUMWORDSTempAndCO2];
#else
	uint16_t _TempBuffer[NUMWORDSTemperature];
	uint16_t _CO2Buffer[NUMWORDSCO2];
#endif
	uint16_t _PressureBuffer[NUMWORDSPressure];
	HardwareSerial* SerialController;
	ModbusMaster MBNode;
	SemaphoreHandle_t _LockMutex;
	bool ReadModbusData(int pAddress, int pNumWords, uint16_t* pBuffer) {
		uint8_t result = MBNode.readHoldingRegisters(pAddress, pNumWords);
		if (result == MBNode.ku8MBSuccess)
		{
			for (int nX = 0; nX < pNumWords; nX++) {
				pBuffer[nX] = MBNode.getResponseBuffer(nX);
			}
			return(true);
		}
		return (false);
	}
	bool WriteModusRegister(int pAddress, uint16_t pValue) {
		uint8_t result = MBNode.writeSingleRegister(pAddress, pValue);
		if (result == MBNode.ku8MBSuccess) {
			return(true);
		}
		return(false);
	}
	void InitModbus(int pPortNum, int8_t pRXPin, int8_t pTXPin) {
		SerialController = new HardwareSerial(pPortNum);
		SerialController->begin(9600, SERIAL_8N1, pRXPin, pTXPin);
		delay(200);
		MBNode.begin(SLAVE_ID, *SerialController);
	}
public:
	/// <summary>
	/// Reads the values from Sensor to the internal buffer
	/// </summary>
	/// <returns>bool.</returns>
	bool ReadValues();
	/// <summary>
	/// Copies the internal values to a given <see cref="EE895_Reading"/>. 
	/// </summary>
	/// <param name="pReading">Pointer to a <see cref="EE895_Reading"/>.</param>
	/// <returns>True if something (not set to ignore) changed.</returns>
	bool ReadDataValues(EE895_Reading* pReading);
	/// <summary>
	/// Sets the c o2 interval.
	/// </summary>
	/// <param name="pSeconds">The seconds.</param>
	/// <returns>bool.</returns>
	bool SetCO2Interval(uint16_t pSeconds);
	/// <summary>
	/// Initializes a new instance of the <see cref="EE895"/> class.
	/// </summary>
	/// <param name="pPortNum">The port number.</param>
	/// <param name="pRXPin">The receive pin.</param>
	/// <param name="pTXPin">The transmit pin.</param>
	EE895(int pPortNum, int8_t pRXPin, int8_t pTXPin) {
		InitModbus(pPortNum, pRXPin, pTXPin);
		_LockMutex = xSemaphoreCreateMutex();
	}
};

bool EE895::ReadValues()
{
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in ReadValues");
		return(false);
	}
	bool bSomethingRead = false;
#ifdef READ_TEMP_AND_CO2_IN_ON_CALL
	bool bResult = ReadModbusData(ADDRTemperature, NUMWORDSTempAndCO2, _TempPlusCO2Buffer);
	if (bResult)
	{
		TempC = _TempPlusCO2Buffer[0] / 100.0;
		TempF = _TempPlusCO2Buffer[1] / 50.0;
		TempK = _TempPlusCO2Buffer[2] / 50.0;

		CO2 = (short)_TempPlusCO2Buffer[NUMWORDSTempAndCO2 - 2];	//CO2 at the end of the buffer
		CO2Raw = (short)_TempPlusCO2Buffer[NUMWORDSTempAndCO2 - 1];
		bSomethingRead = true;
	}
#else
	bool bResult = ReadModbusData(ADDRTemperature, NUMWORDSTemperature, _TempBuffer);
	if (bResult)
	{
		TempC = _TempBuffer[0] / 100.0;
		TempF = _TempBuffer[1] / 50.0;
		TempK = _TempBuffer[2] / 50.0;
		bSomethingRead = true;
	}
	bResult = ReadModbusData(ADDRCO2, NUMWORDSCO2, _CO2Buffer);
	if (bResult)
	{
		CO2 = (short)_CO2Buffer[0];
		CO2Raw = (short)_CO2Buffer[1];
		bSomethingRead = true;
	}
#endif
	bResult = ReadModbusData(ADDRPressure, NUMWORDSPressure, _PressureBuffer);
	if (bResult)
	{
		PressureMBar = _PressureBuffer[0] / 10.0;
		PressurePSI = _PressureBuffer[1] / 100.0;
		bSomethingRead = true;
	}
	xSemaphoreGive(_LockMutex);
	return (bSomethingRead);
}

bool EE895::ReadDataValues(EE895_Reading* pReading) {
	bool bFoundChanges = false;
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in ReadDataValues");
		return(false);
	}
	if (!Ignore_CO2) {
		if (pReading->CO2 != CO2) {
			pReading->CO2 = CO2;
			pReading->CO2_Changed = true;
			bFoundChanges = true;
		}
	}
	if (!Ignore_CO2Raw) {
		if (pReading->CO2Raw != CO2Raw) {
			pReading->CO2Raw = CO2Raw;
			pReading->CO2Raw_Changed = true;
			bFoundChanges = true;
		}
	}
	if (!Ignore_TempC) {
		if (pReading->TempC != TempC) {
			pReading->TempC = TempC;
			pReading->TempC_Changed = true;
			bFoundChanges = true;
		}
	}
	if (!Ignore_TempF) {
		if (pReading->TempF != TempF) {
			pReading->TempF = TempF;
			pReading->TempF_Changed = true;
			bFoundChanges = true;
		}
	}
	if (!Ignore_TempK) {
		if (pReading->TempK != TempK) {
			pReading->TempK = TempK;
			pReading->TempK_Changed = true;
			bFoundChanges = true;
		}
	}
	if (!Ignore_PressureMBar) {
		if (pReading->PressureMBar != PressureMBar) {
			pReading->PressureMBar = PressureMBar;
			pReading->PressureMBar_Changed = true;
			bFoundChanges = true;
		}
	}
	if (!Ignore_PressurePSI) {
		if (pReading->PressurePSI != PressurePSI) {
			pReading->PressurePSI = PressurePSI;
			pReading->PressurePSI_Changed = true;
			bFoundChanges = true;
		}
	}
	xSemaphoreGive(_LockMutex);
	return(bFoundChanges);
}

bool EE895::SetCO2Interval(uint16_t pSeconds)
{
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in SetCO2Interval");
		return(false);
	}
	bool bRet = WriteModusRegister(ADDRCO2Interval, pSeconds * 10);
	xSemaphoreGive(_LockMutex);
	return(bRet);
}

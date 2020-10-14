#pragma once
//#include <Arduino.h>
#include <SPI.h>



// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData1"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL true

// Number length, buffer for storing it and character index
#define NUM_LEN 12
char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

#define ILI9341_DRIVER
//#define TFT_INVERSION_OFF

// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins

#define TFT_CS   5  // Chip select control pin
#define TFT_DC    4  // Data Command control pin
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_RST   22  // Reset pin (could connect to RST pin)
#define TFT_MISO 19

//#define TOUCH_CS 14     // Chip select pin (T_CS) of touch screen


#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
//#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
//#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
//keep this line - else it crashes
#define SMOOTH_FONT

// #define SPI_FREQUENCY  20000000
					   
#define SPI_FREQUENCY  27000000
// #define SPI_FREQUENCY  40000000
// #define SPI_FREQUENCY  55000000 // STM32 SPI1 only (SPI2 maximum is 27MHz)
// #define SPI_FREQUENCY  80000000

// Optional reduced SPI frequency for reading TFT
//#define SPI_READ_FREQUENCY  20000000
#define SPI_READ_FREQUENCY  16000000

// The XPT2046 requires a lower SPI clock rate of 2.5MHz so we define that here:
#define SPI_TOUCH_FREQUENCY  2500000
//ensure our settings take place
#define USER_SETUP_LOADED
#include <TFT_eSPI.h>
#include "./Brightness.h"
#include "./FSMState.h"

//320x240
TFT_eSPI tft = TFT_eSPI();
//XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

TFT_eSPI_Button key[4];

// Keypad start position, key sizes and spacing
#define KEY_X 40 // Centre of key
#define KEY_Y 96
#define KEY_W 62 // Width and height
#define KEY_H 60
#define KEY_H2 60
#define KEY_SPACING_X 12 // X and Y gap
#define KEY_SPACING_X2 18
#define KEY_SPACING_Y 20
#define KEY_TEXTSIZE 1   // Font size multiplier
#define KEY_YOFFS	85

#define TFT_LED 15
//#define TFT_LED_FREQU 30000
#define TFT_LED_FREQU 5000
#define TFT_LED_CHANNEL 2

const char* DATE_TIME_FORMAT = "%e.%b.%Y      %H:%M:%S";

class MyDisplay {
private:
	const uint8_t rotation = 1;
	const int ResultRectWidth = 320;
	const int ResultRectHeight = 50;
	const int PerfectX = 50;
	const int GoodX = 85;
	const int BadX = 110;
	const int AlertX = 85;
	const int YOffset = 6;
	const int TimeX = 15;
	const int TimeY = 224;
	const int CO2X = 85;
	const int CO2Y = 92;
	const int CO2UnitYOffset = 20;
	const int TemperatureX = 250;
	const int TemperatureY = 60;
	const int TemperatureUnitYOffset = 6;
	const int PressureX = 15;
	const int PressureY = 60;
	const int PressureMBarYOffset = 6;
	const int PressurePSIYOffset = 5;
	void DrawKeyFirst() {
		uint8_t row = 0;
		uint8_t col = 0;

		tft.setFreeFont(&FreeSansBold12pt7b);

		key[0].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
			KEY_YOFFS + KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
			KEY_W, KEY_H, TFT_WHITE, ILI9341_BLUE, TFT_WHITE,
			"-", KEY_TEXTSIZE);
		key[0].drawButton();
		col = 1;
		key[1].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
			KEY_YOFFS + KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
			KEY_W, KEY_H, TFT_WHITE, ILI9341_BLUE, TFT_WHITE,
			"+", KEY_TEXTSIZE);
		key[1].drawButton(true);
		col = 2;
		key[2].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
			KEY_YOFFS + KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
			KEY_W, KEY_H2, TFT_WHITE, ILI9341_BLUE, TFT_WHITE,
			"S", KEY_TEXTSIZE);
		key[2].drawButton();
		col = 3;
		key[3].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X2),
			KEY_YOFFS + KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
			KEY_W, KEY_H2, TFT_WHITE, ILI9341_LIGHTGREY, TFT_WHITE,
			"F", KEY_TEXTSIZE);
		key[3].drawButton();
	}
	SemaphoreHandle_t _LockMutex;
	void WriteState(int16_t pXPos, uint16_t pBackColor, uint16_t pTextColor, const char* pText, bool pUseMutex) {
		if (pUseMutex) {
			if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
				Serial.println("Got no mutex in WriteState");
				return;
			}
		}
		tft.fillRect(0, 0, ResultRectWidth, ResultRectHeight, pBackColor); //clear result box
		tft.setCursor(pXPos, YOffset);
		tft.setTextColor(pTextColor);
		tft.setTextSize(2);
		tft.setTextFont(4);
		tft.print(pText);
		if (pUseMutex) {
			xSemaphoreGive(_LockMutex);
		}
	}
	void WriteCO2WithSetMutex(short pValue);
	void WritePressure(float pValue, const char* pUnit, short pUnitOffset) {
		if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
			Serial.println("Got no mutex in WritePressure");
			return;
		}
		tft.setCursor(PressureX, PressureY);
		tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
		tft.setTextSize(1);
		tft.setTextFont(4);
		tft.printf("%.01f", pValue);
		tft.setTextSize(1);
		tft.setTextFont(2);
		tft.setCursor(tft.getCursorX() + 1, PressureY + pUnitOffset);

		tft.print(pUnit);
		xSemaphoreGive(_LockMutex);
	}
	void WriteUnknown(bool pUseMutex) { WriteState(BadX, ILI9341_DARKGREY, ILI9341_BLUE, "???", pUseMutex); }
	void WritePerfect(bool pUseMutex) { WriteState(PerfectX, ILI9341_GREEN, ILI9341_BLACK, "PERFECT", pUseMutex); }
	void WriteGood(bool pUseMutex) { WriteState(GoodX, ILI9341_YELLOW, ILI9341_BLACK, "GOOD", pUseMutex); }
	void WriteBad(bool pUseMutex) { WriteState(BadX, ILI9341_ORANGE, ILI9341_BLACK, "BAD", pUseMutex); }
	void WriteAlert(bool pUseMutex) { WriteState(AlertX, ILI9341_RED, ILI9341_WHITE, "ALERT", pUseMutex); }
	void WriteState(FSMState pState);
	void WriteStateWithSetMutex(FSMState pState);

	// Calibrate the touch screen and retrieve the scaling factors
	//void touch_calibrate();

public:
	void SetDisplayBacklight(Brightness pBright) { ledcWrite(TFT_LED_CHANNEL, 255 - (byte)pBright); }
	void SetDisplayBacklight(byte  pLevel) { SetDisplayBacklight((Brightness)pLevel); }
	void WriteCO2(short pValue);
	void WriteCO2(FSMState pState, short pValue);
	void WriteTemp(float pValue, const char* pUnit);
	void WritePressurePSI(float pValue) { WritePressure(pValue, "PSI", PressurePSIYOffset); }
	void WritePressureMBar(float pValue) { WritePressure(pValue, "mbar", PressureMBarYOffset); }
	void WriteTimeAndDate(struct tm* timeinfo);
	void Init() {
		tft.begin();
		ledcSetup(TFT_LED_CHANNEL, TFT_LED_FREQU, 8);
		ledcAttachPin(TFT_LED, TFT_LED_CHANNEL);
		SetDisplayBacklight(Brightness::NORM_1);
		tft.fillScreen(ILI9341_BLACK);
		tft.setRotation(rotation);
		_LockMutex = xSemaphoreCreateMutex();
		//touch_calibrate();
		//return;
		WriteUnknown(false);
		DrawKeyFirst();
	}
};
void MyDisplay::WriteTimeAndDate(struct tm* timeinfo)
{
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in WriteTimeAndDate");
		return;
	}
	tft.setCursor(TimeX, TimeY);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setTextSize(2);
	tft.setTextFont(1);
	tft.print(timeinfo, DATE_TIME_FORMAT);

	xSemaphoreGive(_LockMutex);
}

inline void MyDisplay::WriteCO2WithSetMutex(short pValue)
{
	char buffer[10];
	if (pValue < 1000) {
		sprintf(buffer, "  %d", pValue);
	}
	else {
		sprintf(buffer, "%d", pValue);
	}
	tft.setCursor(CO2X, CO2Y);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setTextSize(1);
	tft.setTextFont(6);
	tft.printf(buffer);
	tft.setTextSize(2);
	tft.setTextFont(1);
	tft.setCursor(tft.getCursorX() + 3, CO2Y + CO2UnitYOffset);
	tft.print("ppm");
}

void MyDisplay::WriteState(FSMState pState) {
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in WriteState");
		return;
	}
	WriteStateWithSetMutex(pState);
	xSemaphoreGive(_LockMutex);
}

void MyDisplay::WriteStateWithSetMutex(FSMState pState) {
	switch (pState) {
	case FSMState::PERFECT:
		WritePerfect(false);
		return;
	case FSMState::GOOD:
		WriteGood(false);
		return;
	case FSMState::BAD:
		WriteBad(false);
		return;
	case FSMState::ALERT:
		WriteAlert(false);
		return;
	case FSMState::UNKNOWN:
	default:
		WriteUnknown(false);
		return;
	}
}

void MyDisplay::WriteCO2(short pValue) {
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in WriteCO2");
		return;
	}
	WriteCO2WithSetMutex(pValue);
	xSemaphoreGive(_LockMutex);
}

void MyDisplay::WriteCO2(FSMState pState, short pValue) {
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in WriteCO2");
		return;
	}
	WriteStateWithSetMutex(pState);
	WriteCO2WithSetMutex(pValue);
	xSemaphoreGive(_LockMutex);
}

void MyDisplay::WriteTemp(float pValue, const char* pUnit) {
	char buffer[10];
	sprintf(buffer, "%.01f", pValue);
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in WriteTimeAndDate");
		return;
	}
	tft.setCursor(TemperatureX, TemperatureY);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setTextSize(1);
	tft.setTextFont(4);
	tft.printf("%*s", 5 - strlen(buffer), buffer);
	tft.setCursor(tft.getCursorX() + 2, TemperatureY + TemperatureUnitYOffset);
	tft.setTextSize(1);
	tft.setTextFont(2);
	String strUnit = pUnit;
	strUnit.replace("°", "`");	//use degree encoder
	tft.print(strUnit);
	xSemaphoreGive(_LockMutex);
}

#define TS_MINX 230
#define TS_MINY 350
#define TS_MAXX 3700
#define TS_MAXY 3900
uint8_t rot = 1;
int16_t tsx;
int16_t tsy;
void MapTouch(int16_t tsxraw, int16_t tsyraw) {
	switch (rot) {
	case 0: tsx = map(tsyraw, TS_MINY, TS_MAXY, 240, 0);
		tsy = map(tsxraw, TS_MINX, TS_MAXX, 0, 320);
		break;
	case 1: tsx = map(tsxraw, TS_MINX, TS_MAXX, 0, 320);
		tsy = map(tsyraw, TS_MINY, TS_MAXX, 0, 240);
		break;
	case 2: tsx = map(tsyraw, TS_MINY, TS_MAXY, 0, 240);
		tsy = map(tsxraw, TS_MINX, TS_MAXX, 320, 0);
		break;
	case 3: tsx = map(tsxraw, TS_MINX, TS_MAXX, 320, 0);
		tsy = map(tsyraw, TS_MINY, TS_MAXY, 240, 0);
		break;
	}
}
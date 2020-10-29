#pragma once
//#include <Arduino.h>
#include <SPI.h>

#define TS_MINX 220
#define TS_MINY 330
#define TS_MAXX 3650
#define TS_MAXY 3850

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

//avoid loading of touch code by not defining the TOUCH_CS
//#define TOUCH_CS 14     // Chip select pin (T_CS) of touch screen

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
//#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
//#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
//would load a number of fonts - to keep it small only load the one we want
#define _GFXFONT_H_	//simulate included font headers
//unfortunately this is also defined in the "load all free fonts header
typedef struct { // Data stored PER GLYPH
	uint32_t bitmapOffset;     // Pointer into GFXfont->bitmap
	uint8_t  width, height;    // Bitmap dimensions in pixels
	uint8_t  xAdvance;         // Distance to advance cursor (x axis)
	int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;
//
typedef struct { // Data stored for FONT AS A WHOLE:
	uint8_t* bitmap;      // Glyph bitmaps, concatenated
	GFXglyph* glyph;       // Glyph array
	uint16_t  first, last; // ASCII extents
	uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;
//use the local copy of the one and only used font
#include "./FreeSansBold12pt7b.h" // FF26 or FSSB12

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
//keep this line - else it crashes
#define SMOOTH_FONT

// #define SPI_FREQUENCY  20000000
#define SPI_FREQUENCY  27000000

// Optional reduced SPI frequency for reading TFT
//#define SPI_READ_FREQUENCY  20000000
#define SPI_READ_FREQUENCY  16000000

// The XPT2046 requires a lower SPI clock rate of 2.5MHz so we define that here:
//#define SPI_TOUCH_FREQUENCY  2500000
//ensure our settings take place
#define USER_SETUP_LOADED
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h> // Touchscreen Treiber
#include "./Brightness.h"
#include "./FSMState.h"
#include "./MyButton.h"

//320x240
TFT_eSPI tft = TFT_eSPI();

#define MYTOUCH_CS 14
#define MYTOUCH_IRQ 2
XPT2046_Touchscreen _Touch(MYTOUCH_CS);
//XPT2046_Touchscreen _Touch(MYTOUCH_CS, MYTOUCH_IRQ);
#define BRIGHT_DOWN_KEY 1
#define BRIGHT_UP_KEY 2
#define MUTE_KEY 3
#define SWITCH_KEY 4

MyButton* key[4];

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

#define LABEL_Y_OFFS	2

const char* DATE_TIME_FORMAT = "%e.%b.%Y      %H:%M:%S";

class MyDisplay {
private:
	const uint8_t _Rotation = 1;
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
	SemaphoreHandle_t _LockMutex;
	void MapTouch(int16_t& tsxraw, int16_t& tsyraw);
	Brightness _CurBrightness=Brightness::NORM_1;
	void DrawKeyFirst() {
		uint8_t col = 0;
		tft.setFreeFont(&FreeSansBold12pt7b);
		key[0] = new MyButton();
		key[1] = new MyButton();
		key[2] = new ToggleButton();
		key[3] = new ToggleButton();
		key[0]->initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
			KEY_YOFFS + KEY_Y, // x, y, w, h, outline, fill, text
			KEY_W, KEY_H, TFT_WHITE, ILI9341_BLUE, TFT_WHITE, ILI9341_LIGHTGREY,
			"-", KEY_TEXTSIZE);
		key[0]->setLabelDatum(0, LABEL_Y_OFFS);
		key[0]->drawButton();
		col = 1;
		key[1]->initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
			KEY_YOFFS + KEY_Y, // x, y, w, h, outline, fill, text
			KEY_W, KEY_H, TFT_WHITE, ILI9341_BLUE, TFT_WHITE, ILI9341_LIGHTGREY,
			"+", KEY_TEXTSIZE);
		key[1]->setLabelDatum(0, LABEL_Y_OFFS);
		key[1]->drawButton();
		col = 2;
		((ToggleButton*) key[2])->initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
			KEY_YOFFS + KEY_Y, // x, y, w, h, outline, fill, text
			KEY_W, KEY_H2, 
			TFT_WHITE, ILI9341_DARKGREY, TFT_WHITE,
			TFT_WHITE, ILI9341_DARKGREY, TFT_RED,
			ILI9341_LIGHTGREY,
			"S", "M", KEY_TEXTSIZE);
		key[2]->setLabelDatum(0, LABEL_Y_OFFS);
		key[2]->drawButton();
		col = 3;
		((ToggleButton*)key[3])->initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X2),
			KEY_YOFFS + KEY_Y, // x, y, w, h, outline, fill, text
			KEY_W, KEY_H2,
			TFT_WHITE, ILI9341_NAVY, TFT_WHITE,
			TFT_WHITE, ILI9341_DARKGREY, TFT_WHITE,
			ILI9341_LIGHTGREY,
			"Off", "On", KEY_TEXTSIZE);
		key[3]->setLabelDatum(0, LABEL_Y_OFFS);
		key[3]->drawButton();
	}

	void WriteState(int16_t pXPos, uint32_t pBackColor, uint32_t pTextColor, const char* pText, bool pUseMutex) {
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
		Serial.println("State set");
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
	void WriteStateWithSetMutex(FSMState pState);
	void SetCurDisplayBacklight() { ledcWrite(TFT_LED_CHANNEL, 255 - (byte)_CurBrightness); }
	bool WasTouched;
public:
	bool GetButtonStatus(int pButtonNum);
	void SetButtonStatus(int pButtonNum, bool pIsActive);
	void DisOrEnableButton(int pButtonNum, bool pIsEnabled);
	bool IsCurBrightnessFull() {		return(_CurBrightness == Brightness::FULL);	}
	bool IsCurBrightnessOff() { return(_CurBrightness == Brightness::OFF); }
	int HandleTouch();
	void SetDisplayBacklight(Brightness pBright) {
		_CurBrightness = pBright; 
		SetCurDisplayBacklight();
	}
	bool ChangeDisplayBacklight(bool bUp) { 
		if (bUp) {
			if (_CurBrightness == Brightness::FULL) {
				return(false);
			}
			Serial.println(_CurBrightness);
			_CurBrightness++;
			Serial.println(_CurBrightness);
		}
		else{
			if (_CurBrightness == Brightness::OFF) {
				return(false);
			}
			_CurBrightness--;
		}
		SetCurDisplayBacklight();
		return(true);
	}
	void WriteCO2(short pValue);
	void WriteCO2(FSMState pState, short pValue);
	void WriteTemp(float pValue, const char* pUnit);
	void WritePressurePSI(float pValue) { WritePressure(pValue, "PSI", PressurePSIYOffset); }
	void WritePressureMBar(float pValue) { WritePressure(pValue, "mbar", PressureMBarYOffset); }
	void WriteTimeAndDate(struct tm* timeinfo);
	void Init() {
		_Touch.begin();
		_Touch.setRotation(_Rotation);
		tft.begin();
		ledcSetup(TFT_LED_CHANNEL, TFT_LED_FREQU, 8);
		ledcAttachPin(TFT_LED, TFT_LED_CHANNEL);
		SetDisplayBacklight(Brightness::NORM_1);
		tft.fillScreen(ILI9341_BLACK);
		tft.setRotation(_Rotation);
		_LockMutex = xSemaphoreCreateMutex();
		WriteUnknown(false);
		DrawKeyFirst();
	}
};
void MyDisplay::WriteTimeAndDate(struct tm* timeinfo) {
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

inline void MyDisplay::WriteCO2WithSetMutex(short pValue) {
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


inline void MyDisplay::DisOrEnableButton(int pButtonNum, bool pIsEnabled) {
	pButtonNum--;
	if (pButtonNum < 0 || pButtonNum>4) {
		return;
	}
	if (key[pButtonNum]->IsEnabled == pIsEnabled) {
		return;
	}
	key[pButtonNum]->IsEnabled = pIsEnabled;
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in DisOrEnable");
		return;
	}
	tft.setFreeFont(&FreeSansBold12pt7b);
	key[pButtonNum]->drawButton();
	xSemaphoreGive(_LockMutex);
}


void MyDisplay::SetButtonStatus(int pButtonNum, bool pIsActive) {
	//only these two are toggle buttons
	if (pButtonNum != MUTE_KEY && pButtonNum != SWITCH_KEY) {
		return;
	}
	pButtonNum--;
	if (pButtonNum < 0 || pButtonNum>4) {
		return;
	}
	ToggleButton* pButton = (ToggleButton * )key[pButtonNum];
	if (pButton->IsActive() == pIsActive) {
		return;
	}
	pButton->SetActive(pIsActive);
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in DisOrEnable");
		return;
	}
	tft.setFreeFont(&FreeSansBold12pt7b);
	pButton->drawButton();
	xSemaphoreGive(_LockMutex);
}

bool MyDisplay::GetButtonStatus(int pButtonNum) {
	//only these two are toggle buttons
	if (pButtonNum != MUTE_KEY && pButtonNum != SWITCH_KEY) {
		return(false);
	}
	pButtonNum--;
	if (pButtonNum < 0 || pButtonNum>4) {
		return(false);
	}
	return(((ToggleButton*)key[pButtonNum])->IsActive());
}

int MyDisplay::HandleTouch() {
	if ((xSemaphoreTake(_LockMutex, portTICK_PERIOD_MS * 500)) != pdTRUE) {
		Serial.println("Got no mutex in HandleTouch");
		return(0);
	}

	int nRet = 0;
	bool bIsTouched = _Touch.touched();
	TS_Point p = _Touch.getPoint();
	if (bIsTouched) {
		Serial.print("rx = ");
		Serial.print(p.x);
		Serial.print(", ry = ");
		Serial.println(p.y);
	}
	else {
		if (WasTouched) {
			Serial.println("End Touch");
		}
	}
	MapTouch(p.x, p.y);
	if (bIsTouched) {
		Serial.print("x = ");
		Serial.print(p.x);
		Serial.print(", y = ");
		Serial.println(p.y);
	}

	bool bFontSet = false;
	for (uint8_t nX = 0; nX < 4; nX++) {
		if (bIsTouched && key[nX]->contains(p.x, p.y) && key[nX]->IsEnabled) {
			key[nX]->press(true);
		}
		else {
			key[nX]->press(false);
		}
		if (key[nX]->justReleased()) {
			if (nRet == 0) {	//may pressed - which matters more
				nRet = -(nX + 1);
			}
			if (!bFontSet) {
				tft.setFreeFont(&FreeSansBold12pt7b);
				bFontSet = true;
			}
			Serial.printf("Y: %d   H: %d\n", key[nX]->_y1, key[nX]->_h);
			key[nX]->drawButton();
		}
		else if (key[nX]->justPressed()) {
			nRet = nX + 1;

			if (!bFontSet) {
				tft.setFreeFont(&FreeSansBold12pt7b);
				bFontSet = true;
			}
			key[nX]->drawButton(true);
		}
	}
	WasTouched = bIsTouched;
	xSemaphoreGive(_LockMutex);
	return(nRet);
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

void MyDisplay::MapTouch(int16_t& tsxraw, int16_t& tsyraw) {
	int16_t tsx;
	int16_t tsy;

	switch (_Rotation) {
	case 0: tsx = map(tsyraw, TS_MINY, TS_MAXY, 240, 0);
		tsy = map(tsxraw, TS_MINX, TS_MAXX, 0, 320);
		break;
	case 1: tsx = map(tsxraw, TS_MINX, TS_MAXX, 0, 320);
		tsy = map(tsyraw, TS_MINY, TS_MAXY, 0, 240);
		break;
	case 2: tsx = map(tsyraw, TS_MINY, TS_MAXY, 0, 240);
		tsy = map(tsxraw, TS_MINX, TS_MAXX, 320, 0);
		break;
	case 3: tsx = map(tsxraw, TS_MINX, TS_MAXX, 320, 0);
		tsy = map(tsyraw, TS_MINY, TS_MAXY, 240, 0);
		break;
	}
	tsxraw = tsx;
	tsyraw = tsy;
}
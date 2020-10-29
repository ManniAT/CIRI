#pragma once
#define USER_SETUP_LOADED
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#include <TFT_eSPI.h>
class MyButton {
 public:
	 MyButton(void);
	 // "Classic" initButton() uses center & size
	 void initButton(TFT_eSPI* gfx, int16_t x, int16_t y,
		 uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t disabled,
		 uint16_t textcolor, char* label, uint8_t textsize);

	 // New/alt initButton() uses upper-left corner & size
	 void     initButtonUL(TFT_eSPI* gfx, int16_t x1, int16_t y1,
		 uint16_t w, uint16_t h, uint16_t outline, uint16_t fill,
		 uint16_t textcolor, char* label, uint8_t textsize);

	 // Adjust text datum and x, y deltas
	 void     setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);
	 virtual void  drawButton(bool inverted = false) {drawButton(false, "");}
	 void     drawButton(bool inverted, String long_name);
	 bool     contains(int16_t x, int16_t y);

	 void     press(bool p);
	 bool     isPressed();
	 bool     justPressed();
	 bool     justReleased();


	 int16_t  _x1, _y1; // Coordinates of top-left corner of button
	 int16_t  _xd, _yd; // Button text datum offsets (wrt center of button)
	 uint16_t _w, _h;   // Width and height of button
	 bool IsEnabled;
	 
protected:
	uint16_t _outlinecolor, _fillcolor, _textcolor, _disabledColor;
	char     _label[10]; // Button text is 9 chars maximum unless long_name used
private:
	 
	 TFT_eSPI* _gfx;
	 uint8_t  _textsize, _textdatum; // Text size multiplier and text datum for button

	 bool  currstate, laststate; // Button states
};
class ToggleButton : public MyButton {
private:
	uint16_t _outlinecolorActive, _fillcolorActive, _textcolorActive, _disabledColorActive;
	uint16_t _outlinecolorInActive, _fillcolorInActive, _textcolorInActive, _disabledColorInActive;
	bool _IsActive;
	char _labelActive[10];
public:
	
	void initButton(TFT_eSPI* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
		uint16_t outline, uint16_t fill, uint16_t textcolor,
		uint16_t outlineActive, uint16_t fillActive, uint16_t textcolorActive,
		uint16_t disabled,
		char* label, char* labelActive, uint8_t textsize) {
		strncpy(_labelActive, labelActive, 9);
		_outlinecolorInActive = outline;
		_fillcolorInActive = fill;
		_textcolorInActive = textcolor;
		_outlinecolorActive = outlineActive;
		_fillcolorActive = fillActive;
		_textcolorActive = textcolorActive;
		MyButton::initButton(gfx, x, y, w, h, _outlinecolorInActive, _fillcolorInActive, _textcolorInActive, disabled, label, textsize);
	}

	bool IsActive() { return(_IsActive); }
	void SetActive(bool pIsActive) {
		if (_IsActive == pIsActive) {
			return;
		}
		_IsActive = pIsActive;
		if (_IsActive) {
			_outlinecolor = _outlinecolorActive;
			_fillcolor = _fillcolorActive;
			_textcolor = _textcolorActive;
			_disabledColor = _disabledColorActive;
		}
		else {
			_outlinecolor = _outlinecolorInActive;
			_fillcolor = _fillcolorInActive;
			_textcolor = _textcolorInActive;
			_disabledColor = _disabledColorInActive;
		}
	}
	virtual void drawButton(bool inverted = false) {
		MyButton::drawButton(inverted, (_IsActive ? _labelActive : ""));
	}
};

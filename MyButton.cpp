#include "MyButton.h"
/***************************************************************************************
** Code for the GFX button UI element
** Grabbed from Adafruit_GFX library and enhanced to handle any label font
***************************************************************************************/
MyButton::MyButton(void) {
    _gfx = 0;
    _xd = 0;
    _yd = 0;
    _textdatum = MC_DATUM;
}

// Classic initButton() function: pass center & size
void MyButton::initButton(
    TFT_eSPI* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
    uint16_t outline, uint16_t fill, uint16_t textcolor, uint16_t disabled,
    char* label, uint8_t textsize)
{
    _disabledColor = disabled;
    // Tweak arguments and pass to the newer initButtonUL() function...
    initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill,
        textcolor, label, textsize);
}

// Newer function instead accepts upper-left corner & size
void MyButton::initButtonUL(
    TFT_eSPI* gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
    uint16_t outline, uint16_t fill, uint16_t textcolor, 
    char* label, uint8_t textsize)
{
    IsEnabled = true;
    _x1 = x1;
    _y1 = y1;
    _w = w;
    _h = h;
    _outlinecolor = outline;
    _fillcolor = fill;
    _textcolor = textcolor;
    _textsize = textsize;
    _gfx = gfx;
    strncpy(_label, label, 9);
}

// Adjust text datum and x, y deltas
void MyButton::setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum)
{
    _xd = x_delta;
    _yd = y_delta;
    _textdatum = datum;
}

void MyButton::drawButton(bool inverted, String long_name) {
    uint16_t fill, outline, text;
    if (!inverted ) {
        fill = IsEnabled ? _fillcolor : _disabledColor;
        outline = _outlinecolor;
        text = _textcolor;
    }
    else {
        fill = _textcolor;
        outline = _outlinecolor;
        text = _fillcolor;
    }

    uint8_t r = min(_w, _h) / 4; // Corner radius
    _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
    _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

    _gfx->setTextColor(text, fill);
    _gfx->setTextSize(_textsize);

    uint8_t tempdatum = _gfx->getTextDatum();
    _gfx->setTextDatum(_textdatum);
    uint16_t tempPadding = _gfx->padX;
    _gfx->setTextPadding(0);

    if (long_name == "")
        _gfx->drawString(_label, _x1 + (_w / 2) + _xd, _y1 + (_h / 2) - 4 + _yd);
    else
        _gfx->drawString(long_name, _x1 + (_w / 2) + _xd, _y1 + (_h / 2) - 4 + _yd);

    _gfx->setTextDatum(tempdatum);
    _gfx->setTextPadding(tempPadding);
}

bool MyButton::contains(int16_t x, int16_t y) {
    return ((x >= _x1) && (x < (_x1 + _w)) &&
        (y >= _y1) && (y < (_y1 + _h)));
}

void MyButton::press(bool p) {
    laststate = currstate;
    currstate = p;
}

bool MyButton::isPressed() { return currstate; }
bool MyButton::justPressed() { return (currstate && !laststate); }
bool MyButton::justReleased() { return (!currstate && laststate); }

#pragma once
#include <Arduino.h>
#include "lib7segmentfont.h"

#define COMMON_ANODE true
#define COMMON_CATHODE false

#if !(defined(NUMBERS_ONLY) || defined(ALPHANUMERIC_ONLY))
    #define _CHAR_SEARCH_STRING_LENGTH_ 49
    const char PROGMEM _CHAR_SEARCH_STRING_[] = " -.0123456789ABCDEFGHIJLNOPRSTUY?????????????????";
#elif (!defined NUMBERS_ONLY) && (defined ALPHANUMERIC_ONLY)
    #define _CHAR_SEARCH_STRING_LENGTH_ 32
    const char PROGMEM _CHAR_SEARCH_STRING_[] = " -.0123456789ABCDEFGHIJLNOPRSTUY";
#elif (defined NUMBERS_ONLY) && !(defined ALPHANUMERIC_ONLY)
    #define _CHAR_SEARCH_STRING_LENGTH_ 19
    const char PROGMEM _CHAR_SEARCH_STRING_[] =  " -.0123456789ABCDEF";
#endif


class sevenSegmentScreenShifted
{
private:
    PGM_P charSearchString = _CHAR_SEARCH_STRING_;
    byte charSearchStringLength = _CHAR_SEARCH_STRING_LENGTH_;
    byte latchPin;
    byte dataPin; 
    byte clockPin;
    byte numDigits;
    bool commonPin;
    byte pwmPin = 255;
    byte displaySegmentBytes[SCREEN_DIGITS_NUM];
    byte brightness;
public:
    sevenSegmentScreenShifted(byte latchPin, byte dataPin, byte clockPin, bool common);
    sevenSegmentScreenShifted(byte latchPin, byte dataPin, byte clockPin, byte pwmPin, bool common);
    void  clear(void);
    void  setText(const char* text);
    void  renderBytes(const char* text);
    void  setBrightness (byte brightness);
    byte  getBrightness(void);
    byte* getRawBytes(void);
    void  mutate(const char* newPattern, uint8_t digit = 255);
};

// Simple constructor
sevenSegmentScreenShifted::sevenSegmentScreenShifted(byte latchPin, byte dataPin, byte clockPin, bool common = COMMON_ANODE)
{
    this->latchPin  =          latchPin;
    this->dataPin   =           dataPin;
    this->clockPin  =          clockPin;
    this->numDigits = SCREEN_DIGITS_NUM;
    this->commonPin =            common;

    memset(this->displaySegmentBytes, 0, SCREEN_DIGITS_NUM);
}

// Constructor with PWM mode
sevenSegmentScreenShifted::sevenSegmentScreenShifted(byte latchPin, byte dataPin, byte clockPin, byte pwmPin, bool common)
{
    this->latchPin  =          latchPin;
    this->dataPin   =           dataPin;
    this->clockPin  =          clockPin;
    this->numDigits = SCREEN_DIGITS_NUM;
    this->pwmPin    =            pwmPin;
    this->commonPin =            common;
    
    memset(this->displaySegmentBytes, 0, SCREEN_DIGITS_NUM);
}

// We just fill all the digits with charSearchString[0] (i.e. blank space " ")
// just making all bits inverted if we use a common cathode display
void sevenSegmentScreenShifted::clear(void)
{
    digitalWrite(this->latchPin, LOW);
    
    if (this->commonPin == COMMON_ANODE)
        for (byte i = 0; i < this->numDigits; i++)
            shiftOut(this->dataPin, this->clockPin, LSBFIRST, pgm_read_byte(&numberSegmentsFont[0]));
    else    
        for (byte i = 0; i < this->numDigits; i++)
            shiftOut(this->dataPin, this->clockPin, LSBFIRST, ~pgm_read_byte(&numberSegmentsFont[0]));

    digitalWrite(this->latchPin, HIGH);
}

// Populate displaySegmentBytes[] with correct bytes ready to shifting out
void sevenSegmentScreenShifted::renderBytes(const char* text)
{
    byte i = 0;
    byte digitNum = 0;

    // text should not be empty or start with ".". If it does, that's definetly a user's mistake,
    // so we clear the screen and exit.
    if((text[0] == '.') || (text[0] == '\0'))
    {
        this->clear();
        return;
    }

    // Make this->displaySegmentBytes[] contain all bits to manipulate individual segments
    do
    { 
        this->displaySegmentBytes[digitNum] = pgm_read_byte(&numberSegmentsFont[0]); // Set digit to blank space in case there is an unprintable char

        for (byte j = 0; j < this->charSearchStringLength; j++)
        {            
            if (text[i] == '.')
            {
                bitClear(this->displaySegmentBytes[digitNum - 1], 0); // That's only a dot. So we apply it to previous character. 
                
                i++; // Since it'a dot, we'll have to iterate tis symbol one more time
                continue; 
            }

            byte segByte = pgm_read_byte(&this->charSearchString[j]); 
            
            if (segByte == (byte)text[i])
            { 
                this->displaySegmentBytes[digitNum] = pgm_read_byte(&numberSegmentsFont[j]);           
            }
        }

        digitNum++;
        i++;
    } while ((text[i] != '\0') || (digitNum < this->numDigits));

    if (this->commonPin == COMMON_CATHODE)
    {
      for(byte j = 0; j < this->charSearchStringLength; j++)
      {
        this->displaySegmentBytes[j] = ~this->displaySegmentBytes[j];
      }
    }
}

// Here we parse the input text and set it to the shift register
// just making all bits inverted if we use a common cathode display
void sevenSegmentScreenShifted::setText(const char* text)
{
    this->renderBytes(text);

    // Shift out this->displaySegmentBytes[] to 74HC595 registers to make our screen glow!
    // Again, we are making all bits inverted if we use a common cathode display
    for (byte j = this->numDigits; j > 0; j--)
    {
        digitalWrite(this->latchPin, LOW);
            shiftOut(this->dataPin, this->clockPin, LSBFIRST, this->displaySegmentBytes[j - 1]);
        digitalWrite(this->latchPin, HIGH);
    }
}

// We set brightness using one more pin for PWM. Just connect it to
// each 74HC595's OE pin. In case of a common cathode we invert the brightness
void sevenSegmentScreenShifted::setBrightness (byte newBrightness)
{
    this->brightness = newBrightness;
    if(this->pwmPin != 255)
    {
        if(this->commonPin == COMMON_ANODE)
            analogWrite(this->pwmPin, 255 - newBrightness);
        else
            analogWrite(this->pwmPin, newBrightness);
    }
}

byte sevenSegmentScreenShifted::getBrightness(void)
{
    return(this->brightness);
}

byte* sevenSegmentScreenShifted::getRawBytes(void) 
{
  return displaySegmentBytes;
}

// Perform a mutaion on all or only on a selected digit
void sevenSegmentScreenShifted::mutate(const char* newPattern, uint8_t digit = 255)
{
  byte tmpByte = 0;
  for (uint8_t i = 0; i < this->numDigits; i++)
  {
    if((digit == 255) || (digit == i)) 
    {
      uint8_t j;
      for(j = 1; j <= 8; j++)
      {
        bitWrite(tmpByte, (uint8_t)newPattern[j] - 47, bitRead(this->displaySegmentBytes[i], j)); // We substract 47 here to avoid using atoi()
      }
      this->displaySegmentBytes[i] = tmpByte;
    }
  }
}
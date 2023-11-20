#include "settings.h"
#include "Timer3Pin2PWM.h"
#include <WDT.h>
#include <I2C_eeprom.h>

#include "swRTC2000.h"
#include <RtcUtility.h>
#include <RtcDateTime.h>

#ifdef FAST_SHIFT_OUT
  #include <FastShiftOut.h>
  FastShiftOut FSO(DATA, CLOCK, MSBFIRST);
  #define shiftOut(DATA_PIN, CLOCK, ORDER, VALUE) FSO.write(VALUE)
#endif

#define SCREEN_DIGITS_NUM 4
#define NUMBERS_ONLY
#include "lib7SegmentScreenShifted.h"

#include "MoscowSetRise.h"
#include "simpleTimer.h"

swRTC2000 rtc;
char datetime[] = "0000";
byte shiftBytes[5] = {'\0'};
I2C_eeprom EEPROM(0b1010000, I2C_DEVICESIZE_24LC02); //Все адресные ножки 24LC02 подключаем к земле, это даёт нам адрес 0b1010000 или 0x50

#ifdef IV9_NIXIE
  byte* segBytes;
  sevenSegmentScreenShifted IV9Screen(LATCH, DATA, CLOCK, COMMON_CATHODE);
#endif

void setup() 
{
  initTimer3Pin2PWM_32_2000(95, 75);
  wdt_enable(WTO_1S);

  EEPROMValuesInit();

  #ifdef DEBUG_ENABLE
    Serial.begin(115200);
  #endif
  
  pinMode(DATA,    OUTPUT);
  pinMode(LATCH,   OUTPUT);
  pinMode(CLOCK,   OUTPUT);
  pinMode(SW_DOTS, OUTPUT);

  rtc.stopRTC();
    rtc.setDate(17, 12, 2023);
    rtc.setTime(16, 20, 0);
  rtc.startRTC();
}

void loop() 
{
  uint8_t hour = rtc.getHours();
  uint8_t minute = rtc.getMinutes();
  uint8_t second = rtc.getSeconds();
  bool minRefreshFlag = true;
  bool dotRefreshFlag = true;
  Timer16 clockTimer(500);

  for(;;)
  {
    if(clockTimer.ready())
    {
      hour   = rtc.getHours();
      minute = rtc.getMinutes();
      second = rtc.getSeconds();
    }

    if(((second % 2) && dotRefreshFlag))
    {
      analogWrite(SW_DOTS, DOTS_BRIGHTNESS);
      makeDateTimeScreen(datetime, rtc.getHours(), rtc.getMinutes());
      IV9Screen.renderBytes(datetime);
      IV9Screen.mutate(IV9_MUTATION);
      segBytes = IV9Screen.getRawBytes();
      populateIV9(segBytes, shiftBytes);
      dotRefreshFlag = !dotRefreshFlag;

      digitalWrite(LATCH, LOW);
      for (int8_t i = 4; i >= 0; i--)
      {
        shiftOut(DATA, CLOCK, LSBFIRST, shiftBytes[i]);
      }
      digitalWrite(LATCH, HIGH);
    }
    else if((!(second % 2)) && !dotRefreshFlag)
    {
      analogWrite(SW_DOTS, 255);
      makeDateTimeScreen(datetime, rtc.getHours(), rtc.getMinutes());
      IV9Screen.renderBytes(datetime);
      IV9Screen.mutate(IV9_MUTATION);
      segBytes = IV9Screen.getRawBytes();
      populateIV9(segBytes, shiftBytes);
      dotRefreshFlag = !dotRefreshFlag;

      digitalWrite(LATCH, LOW);
      for (int8_t i = 4; i >= 0; i--)
      {
        shiftOut(DATA, CLOCK, LSBFIRST, shiftBytes[i]);
      }
      digitalWrite(LATCH, HIGH);
    }

    if(minute % 5 ) 
    {
      if(minRefreshFlag)
      {
        adjustTime(getGMTOffset());
        minRefreshFlag = false;
        setTimer3Pin2PWMDuty(calculateBrightness());
      }
    }
    wdt_reset();
  }
}

void adjustTime(uint32_t GMTSecondsOffset)
{

}

uint8_t calculateBrightness()
{
  return getDayBrightness();
}